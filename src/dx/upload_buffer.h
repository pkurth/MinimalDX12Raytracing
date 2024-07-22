#pragma once

#include "dx.h"


struct DXAllocation
{
	void* cpu_ptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;

	DXResource resource;
	u64 offset_in_resource;
};

struct DXUploadBuffer
{
	void initialize(u64 capacity);

	DXAllocation allocate(u64 size, u64 alignment);
	void reset() { current = 0; }


	DXResource buffer;
	u8* cpu_base;
	D3D12_GPU_VIRTUAL_ADDRESS gpu_base;

	u64 current = 0;

	std::mutex mutex;
};
