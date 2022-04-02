#include "common.h"
#include "CenterCache.h"
#include "PageCache.h"

CentreCache CentreCache::_sInst;

//size申请的个数,bytes每个的大小
size_t CentreCache::FetchRangeObj(void*& start, void*& end, size_t bytes, size_t number)
{
	//获取下所在的桶
	size_t index = SizeClass().Index(bytes);
	_spanList[index]._mlock.lock();

	Span* span = GetOneSpan(bytes, number);
	assert(span);

	start = span->_freeList;
	//从list 中可以进行读取
	end = start;
	int count = 1, i = 0;
	while (i < number - 1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		i++;
		count++;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanList[index]._mlock.unlock();
	return count;
}

Span* CentreCache::GetOneSpan(size_t bytes, size_t number)
{
	int index = SizeClass().Index(bytes);
	Span* list = _spanList[index]._head;
	Span* cur = list->_next;
	while (cur != list)
	{
		if (cur->_freeList != nullptr)
			return cur;
		cur = cur->_next;
	}

	PageChche::GetInstance()->_mlock.lock();
	//SpanList中没有未分配对象的span，去PageCache申请
	Span*  newSpan = PageChche::GetInstance()->NewSpan(SizeClass::MnmMovePage(bytes));
	PageChche::GetInstance()->_mlock.unlock();
	//将newSpan进行切割
	char* start = (char*)(newSpan->_pagId << kPageShift);
	char* end = start + (newSpan->_npage << kPageShift);

	newSpan->_freeList = start;
	void* tail = start;
	while (start < end)
	{
		NextObj(tail) = start + bytes;
		tail = NextObj(tail);
		start += bytes;
	}
	NextObj(tail) = nullptr;

	_spanList[index].PushFront(newSpan);
	return newSpan;
}