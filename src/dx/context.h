#pragma once

#include "dx.h"
#include "command_queue.h"
#include "resource_graveyard.h"
#include "descriptor_heap.h"
#include "upload_buffer.h"

#include "core/range.h"

struct DXContext
{
	DXContext();
	~DXContext();

	float begin_frame();
	void flush();

	DXCommandList* get_free_render_command_list();
	DXCommandList* get_free_compute_command_list();
	DXCommandList* get_free_copy_command_list();

	u64 execute_command_list(DXCommandList* cl);

	u32 get_free_command_lists_count();
	u32 get_running_command_lists_count();

	void keep_render_resource_alive(DXResource resource);
	void keep_copy_resource_alive(u64 fence_value, DXResource resource);

	DXAllocation allocate_frame_scratch(u64 size, u64 alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	template <typename T>
	DXAllocation push_to_scratch(const T* data, u64 count = 1)
	{
		DXAllocation allocation = allocate_frame_scratch(sizeof(T) * count);
		memcpy(allocation.cpu_ptr, data, sizeof(T) * count);
		return allocation;
	}

	template <typename T>
	DXAllocation push_to_scratch(Range<T> data)
	{
		return push_to_scratch(data.first, data.count);
	}


	u32 wrapping_frame_id() { return (u32)(frame_id % NUM_BUFFERED_FRAMES); }


	u64 frame_id = UINT64_MAX;

	DXFactory factory;
	DXAdapter adapter;
	DXDevice device;

	DXCommandQueue render_queue;
	DXCommandQueue compute_queue;
	DXCommandQueue copy_queue;

private:

	ResourceGraveyard render_graveyard[NUM_BUFFERED_FRAMES];
	ResourceGraveyard copy_graveyard;

	DXUploadBuffer frame_scratch[NUM_BUFFERED_FRAMES];

	std::thread command_list_cleanup_thread;
	HANDLE end_event;

	std::chrono::time_point<std::chrono::high_resolution_clock> last_timepoint;
};

extern DXContext dx_context;
