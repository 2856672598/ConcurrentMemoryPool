#pragma once
#include "common.h"

class ThreadCache
{
public:
	void* AllocateMemory(size_t size);
	void  FreeMemory(void* p, size_t size);

	//从中央缓存获取
	void* FetchFromCentralCache(size_t size);
private:
	FreeList _freelist[NFREELIST];
};

