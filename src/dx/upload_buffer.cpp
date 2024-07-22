#include "upload_buffer.h"

#include "context.h"

void DXUploadBuffer::initialize(u64 capacity)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(capacity);
	CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
	check_dx(dx_context.device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer)
	));

	gpu_base = buffer->GetGPUVirtualAddress();
	buffer->Map(0, 0, (void**)&cpu_base);
}

DXAllocation DXUploadBuffer::allocate(u64 size, u64 alignment)
{
	u64 offset;
	{
		std::lock_guard<std::mutex> lock(mutex);
		offset = align_to(current, alignment);
		current = offset + size;
	}

	DXAllocation result;
	result.cpu_ptr = cpu_base + offset;
	result.gpu_ptr = gpu_base + offset;
	result.resource = buffer;
	result.offset_in_resource = offset;
	return result;
}
