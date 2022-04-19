#pragma once
#include "common.h"
static const size_t ChunkSize = 1024 * 128;
//�����ڴ�

template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* ret = nullptr;
		if (_freeList) {
			ret = (T*)_freeList;
			_freeList = NextObj(_freeList);
		}
		else {
			
			if (_memorySize < sizeof(T)) {
				//ʣ��Ŀռ䲻���ˣ���Ҫȥ����һ���ռ�
				_memory = (char*)SystemAlloc(ChunkSize >> kPageShift);
				_memorySize = ChunkSize;
			}
			//�����и�
			ret = (T*)_memory;
			//�и����СΪһ��ָ��Ĵ�С��
			size_t size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memorySize -= size;
			_memory += size;
		}
		//���ö�λnew���г�ʼ��
		new(ret)T;
		return ret;
	}

	void Delete(T* ptr)
	{
		//�ͷŵ���Դ
		ptr->~T();
		*(void**)ptr = _freeList;
		_freeList = ptr;
	}
private:
	void* _freeList = nullptr;//�ͷŻ����Ŀ�
	char* _memory = nullptr;//����ڴ�
	size_t _memorySize = 0;
};