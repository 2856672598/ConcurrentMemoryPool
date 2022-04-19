#include "common.h"
#include "ThreadCache.h"
#include "CenterCache.h"
#include "PageCache.h"
#include "TraceLog.h"
//����ռ�
void* ThreadCache::AllocateMemory(size_t bytes)
{
	__TRACE_DEBUG("(%u)\n", bytes);
	//���ж���
	bytes = SizeClass::RoundUp(bytes);
	int index = SizeClass::Index(bytes);
	if (!_freelist[index].Empty()) {
		//��������
		return _freelist[index].Pop();
	}
	else {
		//�����뻺������
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
	//����������̫��ʱ��Ҫ���л���
	if (_freelist[index].Length() >= _freelist[index].MaxLength())
	{
		ListTooLong(&_freelist[index], size);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t cl, size_t bytes)
{
	FreeList* list = &_freelist[cl];

	int batch_size = SizeClass::NumMoveSize(bytes);
	int num_to_move = std::min<int>(list->MaxLength(), batch_size);//����ĸ���
	if (num_to_move == list->MaxLength())
		list->MaxLength() += 1;

	void *start = nullptr, *end = nullptr;
	//��ȡ���ĸ���
	int fetch_count = CentreCache::GetInstance()->FetchRangeObj(start, end, bytes, num_to_move);
	assert(fetch_count > 0);
	//���뵽����������
	_freelist[cl].PushRange(start, end, fetch_count);
	return _freelist[cl].Pop();
}
