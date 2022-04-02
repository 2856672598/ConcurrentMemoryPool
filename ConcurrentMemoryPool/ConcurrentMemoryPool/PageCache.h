#pragma once
#include "common.h"

class PageChche
{
public:
	static PageChche* GetInstance()
	{
		return &_sInst;
	}

	Span* NewSpan(size_t n);
	std::mutex _mlock;
private:
	PageChche(const PageChche&) = delete;
	PageChche()
	{}
	SpanList _spanLists[NPAGE];
	static PageChche _sInst;
};