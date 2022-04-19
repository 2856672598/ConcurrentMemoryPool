#include "common.h"
#include "ThreadCache.h"
#include "CenterCache.h"
#include "PageCache.h"
#include "TraceLog.h"
//申请空间
void* ThreadCache::AllocateMemory(size_t bytes)
{
	__TRACE_DEBUG("(%u)\n", bytes);
	//进行对齐
	bytes = SizeClass::RoundUp(bytes);
	int index = SizeClass::Index(bytes);
	if (!_freelist[index].Empty()) {
		//可以申请
		return _freelist[index].Pop();
	}
	else {
		//从中央缓存申请
		return FetchFromCentralCache(index, bytes);
	}

	return nullptr;
}


void ThreadCache::ListTooLong(FreeList* list, int bytes)
{
	//size_t batch_size = SizeClass::NumMoveSize(bytes);
	void* start = nullptr;
	void *end = nullptr;
	list->PopRange(start, end, list->MaxLength());
	CentreCache::GetInstance()->ReleaseToCentralCache(start, bytes);
}

void  ThreadCache::FreeMemory(void* p, size_t size)
{
	size_t index = SizeClass::Index(size);

	_freelist[index].Push(p);
	//当自由链表太长时需要进行回收
	if (_freelist[index].Length() >= _freelist[index].MaxLength())
	{
		ListTooLong(&_freelist[index], size);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t cl, size_t bytes)
{
	FreeList* list = &_freelist[cl];

	int batch_size = SizeClass::NumMoveSize(bytes);
	int num_to_move = std::min<int>(list->MaxLength(), batch_size);//申请的个数
	if (num_to_move == list->MaxLength())
		list->MaxLength() += 1;

	void *start = nullptr, *end = nullptr;
	//获取到的个数
	int fetch_count = CentreCache::GetInstance()->FetchRangeObj(start, end, bytes, num_to_move);
	assert(fetch_count > 0);
	//插入到自由链表中
	_freelist[cl].PushRange(start, end, fetch_count);
	return _freelist[cl].Pop();
}
