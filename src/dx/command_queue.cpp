#include "command_queue.h"

u64 DXCommandQueue::signal()
{
	u64 fence_value = next_fence_value++;
	check_dx(command_queue->Signal(fence.Get(), fence_value));
	return fence_value;
}

bool DXCommandQueue::is_fence_complete(u64 fence_value)
{
	return fence->GetCompletedValue() >= fence_value;
}

void DXCommandQueue::wait_for_fence(u64 fence_value)
{
	if (!is_fence_complete(fence_value))
	{
		HANDLE fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		ASSERT(fence_event);

		fence->SetEventOnCompletion(fence_value, fence_event);
		WaitForSingleObject(fence_event, DWORD_MAX);

		CloseHandle(fence_event);
	}
}

void DXCommandQueue::wait_for_queue(DXCommandQueue& other)
{
	wait_for_queue(other, other.signal());
}

void DXCommandQueue::wait_for_queue(DXCommandQueue& other, u64 fence_value)
{
	command_queue->Wait(other.fence.Get(), fence_value);
}

void DXCommandQueue::wait_for_list(DXCommandList* list)
{
	wait_for_fence(list->last_fence_value);
}

u32 DXCommandQueue::get_free_command_lists_count()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		return free_command_lists.count();
	}
}

u32 DXCommandQueue::get_running_command_lists_count()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		return running_command_lists.count();
	}
}

DXCommandList* DXCommandQueue::get_free_command_list()
{
	DXCommandList* result;

	{
		std::lock_guard<std::mutex> lock(mutex);
		result = free_command_lists.pop_front();
	}

	if (!result)
	{
		std::lock_guard<std::mutex> lock(arena_mutex);
		result = command_list_arena.construct<DXCommandList>(command_list_type);
	}

	return result;
}

u64 DXCommandQueue::execute_command_list(DXCommandList* cl)
{
	check_dx(cl->cl->Close());

	ID3D12CommandList* native_cl = cl->cl.Get();
	command_queue->ExecuteCommandLists(1, &native_cl);

	cl->last_fence_value = signal();

	{
		std::lock_guard<std::mutex> lock(mutex);
		running_command_lists.push_back(cl);
	}

	return cl->last_fence_value;
}

void DXCommandQueue::flush()
{
	while (!running_command_lists.empty()) {}
	wait_for_fence(signal());
}

void DXCommandQueue::add_free_command_list(DXCommandList* cl)
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		free_command_lists.push_front(cl);
	}
}

DXCommandList* DXCommandQueue::peek_oldest_running_command_list()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		return running_command_lists.peek_front();
	}
}

DXCommandList* DXCommandQueue::pop_oldest_running_command_list()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		return running_command_lists.pop_front();
	}
}
