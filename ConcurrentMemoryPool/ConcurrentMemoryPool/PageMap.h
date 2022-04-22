#pragma once
#include "common.h"

template <int BITS>
class TCMalloc_PageMap1
{
public:
	TCMalloc_PageMap1()
	{
		//进行对齐后
		size_t sz = SizeClass::RoundUp(sizeof(void*) << BITS);
		_array = (void**)SystemAlloc(sz >> kPageShift);
		memset(_array, 0, sz);
	}
	// REQUIRES "k" is in range "[0,2^BITS-1]".
	void* get(size_t k) const
	{
		if ((k >> BITS) > 0) {
			return NULL; 
		}
		return _array[k];
	}

	// REQUIRES "k" has been ensured before.
	// 为键 'k' 设置值 'v'。
	//k-->页号 v，span 的地址
	void set(size_t k, void* v)
	{
		_array[k] = v;
	}

private:
	void** _array;
};