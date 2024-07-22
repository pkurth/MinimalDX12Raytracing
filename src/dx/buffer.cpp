#include "buffer.h"
#include "context.h"


static DXGI_FORMAT get_index_buffer_format(u64 element_size)
{
	DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
	if (element_size == 1)
	{
		result = DXGI_FORMAT_R8_UINT;
	}
	else if (element_size == 2)
	{
		result = DXGI_FORMAT_R16_UINT;
	}
	else if (element_size == 4)
	{
		result = DXGI_FORMAT_R32_UINT;
	}
	return result;
}

static std::shared_ptr<DXBuffer> allocate_buffer(u64 element_size, u64 element_count, bool allow_unordered_access, const TCHAR* name, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES initial_state)
{
	D3D12_RESOURCE_FLAGS flags = allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(element_size * element_count, flags);

	DXResource resource;

	CD3DX12_HEAP_PROPERTIES props(heap_type);
	check_dx(dx_context.device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initial_state,
		0,
		IID_PPV_ARGS(&resource)));

	if (name)
	{
		resource->SetName(name);
	}

	std::shared_ptr<DXBuffer> result = std::make_shared<DXBuffer>();
	result->resource = resource;
	result->desc = desc;
	result->element_size = element_size;
	result->element_count = element_count;

	return result;
}

static void upload_buffer_data(std::shared_ptr<DXBuffer> buffer, D3D12_SUBRESOURCE_DATA subresources)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->size());
	DXResource intermediate;

	CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
	check_dx(dx_context.device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(&intermediate)
	));


	DXCommandList* cl = dx_context.get_free_copy_command_list();
	cl->transition(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources(cl->cl.Get(), buffer->resource.Get(), intermediate.Get(), 0, 0, 1, &subresources);

	// We are omitting the transition to common here, since the resource automatically decays to common state after being accessed on a copy queue.
	
	u64 fence_value = dx_context.execute_command_list(cl);

	dx_context.keep_copy_resource_alive(fence_value, intermediate);
}

std::shared_ptr<DXBuffer> create_buffer(const void* data, u64 element_size, u64 element_count, bool allow_unordered_access, const TCHAR* name)
{
	std::shared_ptr<DXBuffer> buffer = allocate_buffer(element_size, element_count, allow_unordered_access, name, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);

	if (data)
	{
		D3D12_SUBRESOURCE_DATA subresource;
		subresource.RowPitch = buffer->size();
		subresource.SlicePitch = buffer->size();
		subresource.pData = data;

		upload_buffer_data(buffer, subresource);
	}

	return buffer;
}

std::shared_ptr<DXBuffer> create_raytracing_buffer(u64 total_size, const TCHAR* name)
{
	std::shared_ptr<DXBuffer> buffer = allocate_buffer(total_size, 1, true, name, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	return buffer;
}

DXVertexBuffer::DXVertexBuffer(std::shared_ptr<DXBuffer> buffer)
	: buffer(buffer)
{
	view.BufferLocation = buffer->virtual_address();
	view.SizeInBytes = (u32)buffer->size();
	view.StrideInBytes = (u32)buffer->element_size;
}

DXIndexBuffer::DXIndexBuffer(std::shared_ptr<DXBuffer> buffer)
	: buffer(buffer)
{
	view.BufferLocation = buffer->virtual_address();
	view.SizeInBytes = (u32)buffer->size();
	view.Format = get_index_buffer_format(buffer->element_size);
}

DXBuffer::~DXBuffer()
{
	if (resource)
	{
		dx_context.keep_render_resource_alive(resource);
	}
}
