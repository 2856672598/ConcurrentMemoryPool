#include "PageCache.h"

PageChche PageChche::_sInst;

Span* PageChche::NewSpan(size_t n)
{
	cout << "申请" << n << "页空间" << endl;
	if (!_spanLists[n].Empty()) {
		return _spanLists[n].PopFront();
	}
	//第n页的位置为空,需要向后面找非空位置进行切割

	for (int i = n; i < NPAGE; i++)
	{
		if (!_spanLists[i].Empty()) {
			//进行切割
			Span* iSpan = _spanLists[i].PopFront();
			Span* span = new Span;
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//将切割后的span 放到指定位置
			_spanLists[iSpan->_npage].PushFront(iSpan);
			return span;
		}
	}

	//没有找到合适的span进行切割-->去系统申请
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = new Span;
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_spanLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}