#pragma once
#include "common.h"
static const size_t ChunkSize = 1024 * 128;
//定长内存

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
				//剩余的空间不够了，需要去申请一块大空间
				_memory = (char*)SystemAlloc(ChunkSize >> kPageShift);
				_memorySize = ChunkSize;
			}
			//进行切割
			ret = (T*)_memory;
			//切割的最小为一个指针的大小。
			size_t size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memorySize -= size;
			_memory += size;
		}
		//调用定位new进行初始化
		new(ret)T;
		return ret;
	}

	void Delete(T* ptr)
	{
		//释放掉资源
		ptr->~T();
		*(void**)ptr = _freeList;
		_freeList = ptr;
	}
private:
	void* _freeList = nullptr;//释放回来的块
	char* _memory = nullptr;//大块内存
	size_t _memorySize = 0;
};