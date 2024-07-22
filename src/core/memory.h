#pragma once

#include "common.h"
#include "range.h"

#define KB(n) (1024ull * (n))
#define MB(n) (1024ull * KB(n))
#define GB(n) (1024ull * MB(n))

template <typename T>
static constexpr T align_to(T offset, T alignment)
{
	T mask = alignment - 1;
	T misalignment = offset & mask;
	T adjustment = (misalignment == 0) ? 0 : (alignment - misalignment);
	return offset + adjustment;
}

struct Arena
{
	Arena(u64 reserve_address_space = GB(1));
	Arena(const Arena&) = delete;
	Arena(Arena&& o) noexcept;
	
	void operator=(const Arena&) = delete;
	void operator=(Arena&& o) noexcept;
	
	~Arena();


	void* allocate(u64 size, u64 alignment = 1, bool clear_to_zero = false);

	template <typename T>
	T* allocate(u64 count = 1, bool clear_to_zero = false)
	{
		return (T*)allocate(sizeof(T) * count, alignof(T), clear_to_zero);
	}

	template <typename T>
	Range<T> allocate_range(u64 count, bool clear_to_zero = false)
	{
		return Range<T>{ allocate<T>(count, clear_to_zero), count };
	}

	template <typename T, typename... Args>
	T* construct(Args&& ...args)
	{
		T* result = allocate<T>(1, false);
		result = new(result) T(std::forward<Args>(args)...);
		return result;
	}

	void align_next_to(u64 alignment);
	void ensure_free(u64 size, u64 alignment = 1);

	void reset();
	void reset_to(u64 offset);




	u8* memory = 0;
	u64 current = 0;
	u64 reserved = 0;
	u64 committed = 0;

	u64 page_size = 0;

	static constexpr u64 minimum_commit_size = KB(4);
};

struct ArenaMarker
{
	ArenaMarker(Arena& arena) : arena(arena), current(arena.current) {}
	ArenaMarker(const ArenaMarker&) = delete;
	ArenaMarker(ArenaMarker&&) = delete;

	void operator=(const ArenaMarker&) = delete;
	void operator=(ArenaMarker&&) = delete;

	~ArenaMarker() { arena.reset_to(current); }

	Arena& arena;
	u64 current;
};

