#pragma once

#include "dx.h"
#include "command_list.h"
#include "core/linked_list.h"
#include "core/memory.h"

struct DXCommandQueue
{
	u64 signal();
	bool is_fence_complete(u64 fence_value);
	void wait_for_fence(u64 fence_value);
	void wait_for_queue(DXCommandQueue& other);
	void wait_for_queue(DXCommandQueue& other, u64 fence_value);
	void wait_for_list(DXCommandList* list);

	u32 get_free_command_lists_count();
	u32 get_running_command_lists_count();

	DXCommandList* get_free_command_list();
	u64 execute_command_list(DXCommandList* cl);

	void flush();

	void add_free_command_list(DXCommandList* cl);
	DXCommandList* peek_oldest_running_command_list();
	DXCommandList* pop_oldest_running_command_list();



	D3D12_COMMAND_LIST_TYPE command_list_type;
	com<ID3D12CommandQueue>	command_queue;
	com<ID3D12Fence> fence;
	std::atomic<u64> next_fence_value = 0;

	LinkedList<DXCommandList> free_command_lists;
	LinkedList<DXCommandList> running_command_lists;

	std::mutex mutex;

	static inline Arena command_list_arena;
	static inline std::mutex arena_mutex;
};
