#include "PageCache.h"

PageChche PageChche::_sInst;

Span* PageChche::NewSpan(size_t n)
{
	if (n > NPAGE - 1) {
		//大于128页的直接向堆申请
		void* ptr = SystemAlloc(n);
		Span* span = _objPool.New();
		//Span* span = new Span;
		span->_pagId = (unsigned)ptr >> kPageShift;
		span->_npage = n;
		//pageNumberToSpanMap[span->_pagId] = span;
		//pageNumberToSpanMap[span->_pagId + span->_npage - 1] = span;

		pageNumberToSpanMap.set(span->_pagId, span);
		pageNumberToSpanMap.set(span->_pagId + span->_npage - 1, span);

		span->_objSize = n * 1024;
		span->_isUse = true;
		return span;
	}

	if (!_pageLists[n].Empty()) {

		Span* nSpan = _pageLists[n].PopFront();
		for (size_t i = 0; i < nSpan->_npage; i++)
		{
			//pageNumberToSpanMap[nSpan->_pagId + i] = nSpan;
			pageNumberToSpanMap.set(nSpan->_pagId + i, nSpan);
		}
		nSpan->_isUse = true;
		return nSpan;
	}
	//第n页的位置为空,需要向后面找非空位置进行切割

	for (int i = n; i < NPAGE; i++)
	{
		if (!_pageLists[i].Empty()) {
			//进行切割
			Span* iSpan = _pageLists[i].PopFront();
			Span* span = _objPool.New();
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//将切割后剩余的iSpan 放到指定位置
			_pageLists[iSpan->_npage].PushFront(iSpan);
			//将切割后剩余的iSpan建立映射,方便进行合并
			//pageNumberToSpanMap[iSpan->_pagId] = iSpan;
			//pageNumberToSpanMap[iSpan->_pagId + iSpan->count - 1] = iSpan;

			pageNumberToSpanMap.set(iSpan->_pagId, iSpan);
			pageNumberToSpanMap.set(iSpan->_pagId + iSpan->count - 1, iSpan);

			//将分出去的span和页号做映射。
			for (size_t i = 0; i < span->_npage; i++)
			{
				pageNumberToSpanMap.set(span->_pagId + i, span);
			}

			span->_isUse = true;//将当前块的状态设置为在使用
			return span;
		}
	}
	//没有找到合适的span进行切割-->去系统申请
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = _objPool.New();
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_pageLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}

//合并数据块
void PageChche::ReleaseToPageCache(Span* span)
{

	if (span->_npage >= NPAGE) {
		//大于NPAGE的直接还给堆
		SystemFree((void*)(span->_pagId >> kPageShift));
		_objPool.Delete(span);
		//delete span;
		return;
	}

	//向前合并
	while (true)
	{
		size_t prevId = span->_pagId - 1;
		Span* prevSpan = (Span*)pageNumberToSpanMap.get(prevId);
		//前面的位置不存在
		if (prevSpan == nullptr) {
			break;
		}

		//前面的span在使用
		//Span* prevSpan = pageNumberToSpanMap[prevId];
		if (prevSpan->_isUse == true || prevSpan->_npage + span->_npage >= NPAGE) {
			break;
		}
		if (prevSpan->_isUse != false)
			int i = 0;
		assert(prevSpan->_isUse == false);
		span->_npage += prevSpan->_npage;
		span->_pagId = prevSpan->_pagId;
		_pageLists[prevSpan->_npage].Pop(prevSpan);
		_objPool.Delete(prevSpan);
		//delete prevSpan;
	}
	//向后合并
	while (true)
	{
		size_t nextId = span->_pagId + span->_npage;
		Span* nextSpan = (Span*)pageNumberToSpanMap.get(nextId);
		if (nextSpan == nullptr) {
			break;
		}
		//Span* nextSpan = pageNumberToSpanMap[nextId];
		if (nextSpan->_isUse == true || nextSpan->_npage + span->_npage >= NPAGE) {
			break;
		}

		_pageLists[nextSpan->_npage].Pop(nextSpan);
		//可以进行合并
		span->_npage += nextSpan->_npage;
		_objPool.Delete(nextSpan);
		//delete nextSpan;
	}

	//将合并好的span添加到对应的桶中
	_pageLists[span->_npage].PushFront(span);
	span->_isUse = false;
	//pageNumberToSpanMap[span->_pagId] = span;
	//pageNumberToSpanMap[span->_pagId + span->_npage - 1] = span;

	pageNumberToSpanMap.set(span->_pagId, span);
	pageNumberToSpanMap.set(span->_pagId + span->_npage - 1, span);
}