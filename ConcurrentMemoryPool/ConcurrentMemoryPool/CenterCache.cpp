#include "common.h"
#include "CenterCache.h"
#include "PageCache.h"

CentreCache CentreCache::_sInst;

//size����ĸ���,bytesÿ���Ĵ�С
size_t CentreCache::FetchRangeObj(void*& start, void*& end, size_t bytes, size_t number)
{
	//��ȡ�����ڵ�Ͱ
	size_t index = SizeClass().Index(bytes);
	_spanList[index]._mlock.lock();

	Span* span = GetOneSpan(bytes, number);
	assert(span);

	start = span->_freeList;
	//��list �п��Խ��ж�ȡ
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
	//SpanList��û��δ��������span��ȥPageCache����
	Span*  newSpan = PageChche::GetInstance()->NewSpan(SizeClass::MnmMovePage(bytes));
	PageChche::GetInstance()->_mlock.unlock();
	//��newSpan�����и�
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