#pragma once

#include "dx.h"
#include "descriptor.h"
#include "core/memory.h"
#include "core/linked_list.h"

struct DXDescriptorHeap;

struct DXDescriptorAllocation
{
	inline CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_at(u32 index = 0) const { ASSERT(index < count); return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpu_base, index, descriptor_size); }
	inline CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_at(u32 index = 0) const { ASSERT(index < count); return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpu_base, index, descriptor_size); }

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_base;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_base;
	u32 count = 0;
	u32 descriptor_size;
};

struct DXDescriptorRange
{
	DXDescriptorRange* next;

	u32 offset;
	u32 count;
};

struct DXDescriptorHeap
{
	void initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shader_visible, u32 capacity);

	DXDescriptorAllocation allocate(u32 count = 1);

	D3D12_DESCRIPTOR_HEAP_TYPE type;
	com<ID3D12DescriptorHeap> heap;

	bool shader_visible;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_base;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_base;
	u32 descriptor_size;

private:

	u32 current = 0;

	std::mutex mutex;
};
