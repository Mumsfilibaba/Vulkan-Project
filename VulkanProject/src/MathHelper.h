#pragma once
#include "Core.h"

namespace Math
{
	template<typename T>
	inline T AlignUp(T value, size_t alignment)
	{
		size_t mask = alignment - 1;
		return (T)(((size_t)value + mask) & ~mask);
	}

	template<typename T>
	inline T AlignDown(T value, size_t alignment)
	{
		size_t mask = alignment - 1;
		return (T)((size_t)value & ~mask);
	}
}