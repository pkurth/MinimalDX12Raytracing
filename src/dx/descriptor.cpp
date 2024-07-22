#include "descriptor.h"
#include "context.h"


static u32 get_channel_count(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
			return 1;
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return 2;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return 3;
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4;


		case DXGI_FORMAT_BC4_UNORM:
			return 1;
		case DXGI_FORMAT_BC5_UNORM:
			return 2;
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return 3;
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return 4;

		default:
			//ASSERT(false);
			return 0;
	}
}

static u32 get_shader_4_component_mapping(DXGI_FORMAT format)
{
	switch (get_channel_count(format))
	{
		case 1:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 2:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 3:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 4:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
	}
	return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
}


void create_2d_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range, DXGI_FORMAT override_format)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? resource->GetDesc().Format : override_format;
	desc.Shader4ComponentMapping = get_shader_4_component_mapping(desc.Format);
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MostDetailedMip = mip_range.first;
	desc.Texture2D.MipLevels = mip_range.count;

	dx_context.device->CreateShaderResourceView(resource.Get(), &desc, handle);
}

void create_cubemap_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range, u32 first_cube, u32 cube_count, DXGI_FORMAT override_format)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? resource->GetDesc().Format : override_format;
	desc.Shader4ComponentMapping = get_shader_4_component_mapping(desc.Format);
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	desc.TextureCubeArray.MostDetailedMip = mip_range.first;
	desc.TextureCubeArray.MipLevels = mip_range.count;
	desc.TextureCubeArray.NumCubes = cube_count;
	desc.TextureCubeArray.First2DArrayFace = first_cube * 6;

	dx_context.device->CreateShaderResourceView(resource.Get(), &desc, handle);
}

void create_volume_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range, DXGI_FORMAT override_format)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = (override_format == DXGI_FORMAT_UNKNOWN) ? resource->GetDesc().Format : override_format;
	desc.Shader4ComponentMapping = get_shader_4_component_mapping(desc.Format);
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	desc.Texture3D.MostDetailedMip = mip_range.first;
	desc.Texture3D.MipLevels = mip_range.count;
	desc.Texture3D.ResourceMinLODClamp = 0;

	dx_context.device->CreateShaderResourceView(resource.Get(), &desc, handle);
}

void create_2d_texture_uav(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, u32 mip_slice, DXGI_FORMAT override_format)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = override_format;
	desc.Texture2D.MipSlice = mip_slice;

	dx_context.device->CreateUnorderedAccessView(resource.Get(), 0, &desc, handle);
}

void create_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, BufferRange range)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = (u32)range.first;
	desc.Buffer.NumElements = (u32)range.count;
	desc.Buffer.StructureByteStride = (u32)range.element_size;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	dx_context.device->CreateShaderResourceView(resource.Get(), &desc, handle);
}

void create_raw_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, BufferRange range)
{
	u64 first_element_offset = range.first * range.element_size;
	ASSERT(first_element_offset % 16 == 0);

	u64 total_size = range.count * range.element_size;

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = first_element_offset / 4;
	desc.Buffer.NumElements = (u32)(total_size / 4);
	desc.Buffer.StructureByteStride = 0;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	dx_context.device->CreateShaderResourceView(resource.Get(), &desc, handle);
}

void create_buffer_uav(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, BufferRange range)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Buffer.CounterOffsetInBytes = 0;
	desc.Buffer.FirstElement = (u32)range.first;
	desc.Buffer.NumElements = (u32)range.count;
	desc.Buffer.StructureByteStride = (u32)range.element_size;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	dx_context.device->CreateUnorderedAccessView(resource.Get(), 0, &desc, handle);
}

void create_raytracing_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.RaytracingAccelerationStructure.Location = resource->GetGPUVirtualAddress();

	dx_context.device->CreateShaderResourceView(nullptr, &desc, handle);
}
