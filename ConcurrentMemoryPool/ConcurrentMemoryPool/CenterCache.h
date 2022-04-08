#pragma once
#include "common.h"

//中心缓存-->单例模式
class CentreCache
{
public:
	CentreCache(const CentreCache&) = delete;
	static CentreCache* GetInstance()
	{
		return &_sInst;
	}

	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
	Span* GetOneSpan(size_t bytes, size_t number);
	void ReleaseToCentralCache(void* begin, int bytes);

private:
	CentreCache()
	{}

	static CentreCache _sInst;
	SpanList _spanList[NSPANLIST];
};
