#pragma once

#include "command_queue.h"
#include "descriptor_heap.h"

struct ResourceGrave
{
	ResourceGrave* next;

	DXResource resource;

	u64 fence_value;
};

struct ResourceGraveyard
{
	void add_resource(u64 fence_value, DXResource resource);
	void cleanup();

	DXCommandQueue* queue;
	Arena arena;
	LinkedList<ResourceGrave> free_graves;
	LinkedList<ResourceGrave> full_graves;
	std::mutex mutex;
};
