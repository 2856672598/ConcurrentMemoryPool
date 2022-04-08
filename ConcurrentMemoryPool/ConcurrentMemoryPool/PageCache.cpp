#include "PageCache.h"

PageChche PageChche::_sInst;

Span* PageChche::NewSpan(size_t n)
{
	if (!_pageLists[n].Empty()) {

		Span* nSpan = _pageLists[n].PopFront();
		for (size_t i = 0; i < nSpan->_npage; i++)
		{
			pageNumberToSpanMap[nSpan->_pagId + i] = nSpan;
		}
		return nSpan;
	}
	//第n页的位置为空,需要向后面找非空位置进行切割

	for (int i = n; i < NPAGE; i++)
	{
		if (!_pageLists[i].Empty()) {
			//进行切割
			Span* iSpan = _pageLists[i].PopFront();
			Span* span = new Span;
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//将切割后剩余的iSpan 放到指定位置
			_pageLists[iSpan->_npage].PushFront(iSpan);
			//将切割后剩余的iSpan建立映射,方便进行合并
			pageNumberToSpanMap[iSpan->_pagId] = iSpan;
			pageNumberToSpanMap[iSpan->_pagId + iSpan->count - 1] = iSpan;

			//将分出去的span和页号做映射。
			for (size_t i = 0; i < span->_npage; i++)
			{
				pageNumberToSpanMap[span->_pagId + i] = span;
			}

			return span;
		}
	}
	cout << "申请" << n << "页空间" << endl;
	//没有找到合适的span进行切割-->去系统申请
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = new Span;
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_pageLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}