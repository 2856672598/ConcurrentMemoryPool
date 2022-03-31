#include "common.h"
#include "ThreadCache.h"

//����ռ�
void* ThreadCache::AllocateMemory(size_t size)
{
	//���ж���
	size = SizeClass::RoundUp(size);

	int index = SizeClass().Index(size);
	if (!_freelist[index].Empty()) {
		//��������
		return _freelist[index].Pop();
	}
	else {
		//�����뻺������
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
