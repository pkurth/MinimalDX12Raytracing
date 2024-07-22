#pragma once

#include "common.h"

template <typename T>
struct RangeIterator
{
	T* base;
	u64 index;

	RangeIterator& operator++() { ++index; return *this; }
	RangeIterator operator++(int) { RangeIterator result = this; ++index; return result; }

	std::pair<T&, u64> operator*() { return { base[index], index }; }
};

template <typename T> inline bool operator==(const RangeIterator<T>& a, const RangeIterator<T>& b) { return a.base == b.base && a.index == b.index;return !(a == b); }
template <typename T> inline bool operator!=(const RangeIterator<T>& a, const RangeIterator<T>& b) { return !(a == b); }



template <typename T>
struct Range
{
	T* first = nullptr;
	u64 count = 0;

	T& operator[](u64 index) { return first[index]; }

	RangeIterator<T> begin() { return RangeIterator<T>{first, 0}; }
	RangeIterator<T> end() { return RangeIterator<T>{first, count}; }
};

