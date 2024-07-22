#include "texture.h"
#include "context.h"

#include <cmath>

static u32 get_format_size(DXGI_FORMAT format)
{
	u32 size = 0;

	switch (format)
	{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			size = 4 * 4;
			break;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
			size = 3 * 4;
			break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			size = 4 * 2;
			break;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			size = 2 * 4;
			break;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			size = 4;
			break;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			size = 4;
			break;
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
			size = 2 * 2;
			break;
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
			size = 4;
			break;
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
			size = 2;
			break;
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
			size = 2;
			break;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
			size = 1;
			break;
			size = 4;
			break;

		default:
			ASSERT(false); // Compressed format.
	}

	return size;
}

static bool check_format_support(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support, D3D12_FORMAT_SUPPORT1 support)
{
	return (format_support.Support1 & support) != 0;
}

static bool check_format_support(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support, D3D12_FORMAT_SUPPORT2 support)
{
	return (format_support.Support2 & support) != 0;
}

static bool format_supports_rtv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
{
	return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
}

static bool format_supports_dsv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
{
	return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
}

static bool format_supports_srv(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
{
	return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
}

static bool format_supports_uav(D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support)
{
	return check_format_support(format_support, D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
		check_format_support(format_support, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
		check_format_support(format_support, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
}

static std::shared_ptr<DXTexture> allocate_texture(D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES initial_state, const TCHAR* name = nullptr)
{
	u32 max_mip_levels = (u32)log2((float)max((u32)desc.Width, desc.Height)) + 1;
	desc.MipLevels = min<u32>(max_mip_levels, desc.MipLevels);

	DXResource resource;

	auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	check_dx(dx_context.device->CreateCommittedResource(&heap_desc,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initial_state,
		0,
		IID_PPV_ARGS(&resource)));

	if (name)
	{
		resource->SetName(name);
	}

	desc = resource->GetDesc();

	std::shared_ptr<DXTexture> result = std::make_shared<DXTexture>();
	result->resource = resource;
	result->desc = desc;



	D3D12_FEATURE_DATA_FORMAT_SUPPORT format_support;
	format_support.Format = desc.Format;
	check_dx(dx_context.device->CheckFeatureSupport(
		D3D12_FEATURE_FORMAT_SUPPORT,
		&format_support,
		sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));

	return result;
}

static void upload_texture_data(std::shared_ptr<DXTexture> texture, D3D12_SUBRESOURCE_DATA* subresources, u32 first_subresource, u32 subresource_count)
{
	u64 required_intermediate_size = GetRequiredIntermediateSize(texture->resource.Get(), first_subresource, subresource_count);

	auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(required_intermediate_size);
	DXResource intermediate;

	auto heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	check_dx(dx_context.device->CreateCommittedResource(
		&heap_desc,
		D3D12_HEAP_FLAG_NONE,
		&buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(&intermediate)
	));


	DXCommandList* cl = dx_context.get_free_copy_command_list();
	cl->transition(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources<128>(cl->cl.Get(), texture->resource.Get(), intermediate.Get(), 0, first_subresource, subresource_count, subresources);
	u64 fence_value = dx_context.execute_command_list(cl);

	dx_context.keep_copy_resource_alive(fence_value, intermediate);
}

std::shared_ptr<DXTexture> create_texture_from_data(void* data, u32 width, u32 height, DXGI_FORMAT format, bool allow_unordered_access, const TCHAR* name)
{
	D3D12_RESOURCE_FLAGS flags = allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, flags);

	std::shared_ptr<DXTexture> texture = allocate_texture(desc, D3D12_RESOURCE_STATE_COMMON, name);

	if (data)
	{
		u32 formatSize = get_format_size(desc.Format);

		D3D12_SUBRESOURCE_DATA subresource;
		subresource.RowPitch = width * formatSize;
		subresource.SlicePitch = width * height * formatSize;
		subresource.pData = data;

		upload_texture_data(texture, &subresource, 0, 1);
	}

	return texture;
}

DXTexture::~DXTexture()
{
	if (resource)
	{
		dx_context.keep_render_resource_alive(resource);
	}
}
