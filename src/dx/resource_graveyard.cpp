#include "resource_graveyard.h"

void ResourceGraveyard::add_resource(u64 fence_value, DXResource resource)
{
	std::lock_guard<std::mutex> lock(mutex);
	ResourceGrave* grave = free_graves.pop_front();

	if (!grave)
	{
		grave = arena.allocate<ResourceGrave>(1);
	}

	grave->fence_value = fence_value;
	grave->resource = resource;

	full_graves.push_back(grave);
}

void ResourceGraveyard::cleanup()
{
	std::lock_guard<std::mutex> lock(mutex);

	ResourceGrave* before = nullptr;
	for (ResourceGrave* grave = full_graves.first; grave; grave = grave->next)
	{
		if (queue->is_fence_complete(grave->fence_value))
		{
			//std::cout << "Releasing resource\n";
			grave->resource.Reset();

			full_graves.remove(grave, before);
			free_graves.push_back(grave);
		}

		before = grave;
	}
}


