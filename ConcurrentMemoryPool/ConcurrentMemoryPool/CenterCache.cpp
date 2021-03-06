#include "common.h"
#include "CenterCache.h"
#include "PageCache.h"

CentreCache CentreCache::_sInst;

//size申请的个数,bytes每个的大小
size_t CentreCache::FetchRangeObj(void*& start, void*& end, size_t bytes, size_t number)
{
	//获取下所在的桶
	size_t index = SizeClass::Index(bytes);
	_spanList[index]._mlock.lock();

	Span* span = GetOneSpan(bytes, number);

	assert(span);
	assert(span->_freeList);
	span->_objSize = bytes;
	start = span->_freeList;
	//从list 中可以进行读取
	end = start;
	size_t count = 1, i = 0;
	while (i < number - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		i++;
		count++;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->count += count;
	_spanList[index]._mlock.unlock();
	return count;
}

Span* CentreCache::GetOneSpan(size_t bytes, size_t number)
{
	int index = SizeClass::Index(bytes);
	Span* head = _spanList[index]._head;//头结点
	Span* cur = head->_next;
	
	while (cur != head)
	{
		if (cur->_freeList != nullptr)
			return cur;
		cur = cur->_next;
	}

	
	PageChche::GetInstance()->_mlock.lock();
	//SpanList中没有未分配对象的span，去PageCache申请
	Span*  newSpan = PageChche::GetInstance()->NewSpan(SizeClass::NnmMovePage(bytes));
	PageChche::GetInstance()->_mlock.unlock();
	//将newSpan进行切割
	newSpan->_objSize = bytes;
	char* start = (char*)(newSpan->_pagId << kPageShift);
	char* end = start + (newSpan->_npage << kPageShift);
	

	newSpan->_freeList = start;
	void* tail = start;
	while (start + bytes < end)
	{
		NextObj(tail) = start + bytes;
		tail = NextObj(tail);
		start += bytes;
		//newSpan->count++;
	}
	NextObj(tail) = nullptr;

	_spanList[index].PushFront(newSpan);
	return newSpan;
}

void CentreCache::ReleaseToCentralCache(void* begin, int bytes)
{

	//访问CentreCache时，需要加上桶锁。
	int index = SizeClass::Index(bytes);
	_spanList[index]._mlock.lock();
	void* cur = begin;
	while (cur)
	{
		void* next = NextObj(cur);

		Span* span = PageChche::GetInstance()->AddrToSpan(cur);
		NextObj(cur) = span->_freeList;
		span->_freeList = cur;
		span->count--;//回收节点进行--

		if (span->count == 0) {
			_spanList[index].Pop(span);

			span->_freeList = nullptr;
			span->_next = span->_prev = nullptr;
			span->_objSize = 0;

			_spanList[index]._mlock.unlock();
			//全部回收完成-->还给下一层的page
			PageChche::GetInstance()->_mlock.lock();
			PageChche::GetInstance()->ReleaseToPageCache(span);
			PageChche::GetInstance()->_mlock.unlock();

			_spanList[index]._mlock.lock();

		}
		cur = next;
	}

	_spanList[index]._mlock.unlock();
}