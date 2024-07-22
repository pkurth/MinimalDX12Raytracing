#pragma once

#include <cinttypes>
#include <cassert>

#include <mutex>
#include <iostream>
#include <filesystem>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef wchar_t wchar;

#define ASSERT(x) assert(x)

template <typename T>
constexpr inline auto min(T a, T b)
{
	return (a < b) ? a : b;
}

template <typename T>
constexpr inline auto max(T a, T b)
{
	return (a < b) ? b : a;
}

#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))


