#include "common.h"
#include "ThreadCache.h"
#include "CenterCache.h"

//����ռ�
void* ThreadCache::AllocateMemory(size_t bytes)
{
	//���ж���
	bytes = SizeClass::RoundUp(bytes);

	int index = SizeClass().Index(bytes);
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

void  ThreadCache::FreeMemory(void* p, size_t size)
{
	size_t index = SizeClass().Index(size);
	_freelist[index].Push(p);
}

void* ThreadCache::FetchFromCentralCache(size_t cl, size_t bytes)
{
	FreeList* list = &_freelist[cl];

	int batch_size = SizeClass::NumMoveSize(bytes);
	int num_to_move = std::min<int>(list->length(), batch_size);//����ĸ���

	void *start, *end;
	//��ȡ���ĸ���
	int fetch_count = CentreCache::GetInstance()->FetchRangeObj(start, end, bytes, num_to_move);
	assert(fetch_count > 0);
	//���뵽����������
	_freelist[cl].PushRange(start, end, fetch_count);
	return _freelist[cl].Pop();
}
