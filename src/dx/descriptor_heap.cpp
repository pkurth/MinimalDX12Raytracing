#include "descriptor_heap.h"
#include "context.h"

void DXDescriptorHeap::initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shader_visible, u64 capacity)
{
	this->type = type;
	this->shader_visible = shader_visible;
	this->capacity = capacity;


	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = (u32)capacity;
	desc.Type = type;
	desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	check_dx(dx_context.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	cpu_base = heap->GetCPUDescriptorHandleForHeapStart();
	gpu_base = shader_visible ? heap->GetGPUDescriptorHandleForHeapStart() : CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	descriptor_size = dx_context.device->GetDescriptorHandleIncrementSize(type);
}

DXDescriptorAllocation DXDescriptorHeap::allocate(u64 count)
{
	std::lock_guard<std::mutex> lock(mutex);

	ASSERT(current + count <= capacity);

	DXDescriptorAllocation allocation;
	allocation.cpu_base = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_base, (u32)current, descriptor_size);
	allocation.gpu_base = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_base, (u32)current, descriptor_size);
	allocation.count = (u32)count;
	allocation.descriptor_size = descriptor_size;

	current += count;

	return allocation;
}
