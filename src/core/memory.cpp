#include "memory.h"

#include <utility>

Arena::Arena(u64 reserve_address_space)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	page_size = systemInfo.dwPageSize;

	reserved = align_to(reserve_address_space, page_size);
	memory = (u8*)VirtualAlloc(0, reserved, MEM_RESERVE, PAGE_READWRITE);
}

Arena::Arena(Arena&& o) noexcept
{
	*this = std::move(o);
}

void Arena::operator=(Arena&& o) noexcept
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}

	memory = std::exchange(o.memory, nullptr);
	reserved = o.reserved;
	current = o.current;
	committed = o.committed;
	page_size = o.page_size;
}

Arena::~Arena()
{
	if (memory)
	{
		VirtualFree(memory, 0, MEM_RELEASE);
	}
}

void* Arena::allocate(u64 size, u64 alignment, bool clear_to_zero)
{
	if (!memory || size == 0)
	{
		return 0;
	}

	ensure_free(size, alignment);

	current = align_to(current, alignment);

	u8* result = memory + current;
	current += size;

	if (clear_to_zero)
	{
		memset(result, 0, size);
	}

	return result;
}

void Arena::align_next_to(u64 alignment)
{
	ensure_free(0, alignment);
	current = align_to(current, alignment);
}

void Arena::ensure_free(u64 size, u64 alignment)
{
	u64 aligned_current = align_to(current, alignment);
	ASSERT(aligned_current + size <= reserved);

	if (aligned_current + size > committed)
	{
		u64 allocation = aligned_current + size - committed;
		allocation = max(allocation, minimum_commit_size);
		allocation = align_to(allocation, page_size);

		VirtualAlloc(memory + committed, allocation, MEM_COMMIT, PAGE_READWRITE);

		committed += allocation;
	}
}

void Arena::reset()
{
	reset_to(0);
}

void Arena::reset_to(u64 offset)
{
	ASSERT(offset <= current);
	current = offset;
}

