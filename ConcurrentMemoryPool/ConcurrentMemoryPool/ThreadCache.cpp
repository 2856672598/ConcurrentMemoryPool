#include "common.h"
#include "ThreadCache.h"

//申请空间
void* ThreadCache::AllocateMemory(size_t size)
{
	//进行对齐
	size = SizeClass::RoundUp(size);

	int index = SizeClass().Index(size);
	if (!_freelist[index].Empty()) {
		//可以申请
		return _freelist[index].Pop();
	}
	else {
		//从中央缓存申请
		return FetchFromCentralCache(size);
	}

	return nullptr;
}

void  ThreadCache::FreeMemory(void* p, size_t size)
{
	size_t index = SizeClass().Index(size);
	_freelist[index].Push(p);
}

void* ThreadCache::FetchFromCentralCache(size_t size)
{
	return nullptr;
}
