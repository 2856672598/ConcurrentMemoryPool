#include "common.h"
#include "CenterCache.h"
#include "PageCache.h"

CentreCache CentreCache::_sInst;

//size����ĸ���,bytesÿ���Ĵ�С
size_t CentreCache::FetchRangeObj(void*& start, void*& end, size_t bytes, size_t number)
{
	//��ȡ�����ڵ�Ͱ
	size_t index = SizeClass::Index(bytes);
	_spanList[index]._mlock.lock();

	Span* span = GetOneSpan(bytes, number);

	assert(span);
	assert(span->_freeList);
	span->_objSize = bytes;
	start = span->_freeList;
	//��list �п��Խ��ж�ȡ
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
	Span* head = _spanList[index]._head;//ͷ���
	Span* cur = head->_next;
	
	while (cur != head)
	{
		if (cur->_freeList != nullptr)
			return cur;
		cur = cur->_next;
	}

	
	PageChche::GetInstance()->_mlock.lock();
	//SpanList��û��δ��������span��ȥPageCache����
	Span*  newSpan = PageChche::GetInstance()->NewSpan(SizeClass::NnmMovePage(bytes));
	PageChche::GetInstance()->_mlock.unlock();
	//��newSpan�����и�
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

	//����CentreCacheʱ����Ҫ����Ͱ����
	int index = SizeClass::Index(bytes);
	_spanList[index]._mlock.lock();
	void* cur = begin;
	while (cur)
	{
		void* next = NextObj(cur);

		Span* span = PageChche::GetInstance()->AddrToSpan(cur);
		NextObj(cur) = span->_freeList;
		span->_freeList = cur;
		span->count--;//���սڵ����--

		if (span->count == 0) {
			_spanList[index].Pop(span);

			span->_freeList = nullptr;
			span->_next = span->_prev = nullptr;
			span->_objSize = 0;

			_spanList[index]._mlock.unlock();
			//ȫ���������-->������һ���page
			PageChche::GetInstance()->_mlock.lock();
			PageChche::GetInstance()->ReleaseToPageCache(span);
			PageChche::GetInstance()->_mlock.unlock();

			_spanList[index]._mlock.lock();

		}
		cur = next;
	}

	_spanList[index]._mlock.unlock();
}