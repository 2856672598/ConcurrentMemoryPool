#pragma once
#include "common.h"
#include "ObjectPool.h"
#include "PageMap.h"


class PageChche
{
public:
	std::mutex _mlock;
	static PageChche* GetInstance()
	{
		return &_sInst;
	}

	Span* NewSpan(size_t n);
	void ReleaseToPageCache(Span* span);

	Span* AddrToSpan(void* addr)
	{
		//std::unique_lock<std::mutex> lock(_mlock);
		////����ַת��Ϊҳ��
		//size_t pageId = (size_t)addr >> kPageShift;
		//auto ret = pageNumberToSpanMap.find(pageId);
		//if (ret != pageNumberToSpanMap.end()) {
		//	return ret->second;
		//}
		////˵��ǰ���߼�������
		//assert(false);
		//return nullptr;

		size_t pageId = (size_t)addr >> kPageShift;
		auto ret = (Span*)pageNumberToSpanMap.get(pageId);
		assert(ret != nullptr);
		return ret;
	}

private:
	PageChche(const PageChche&) = delete;
	PageChche()
	{}
	SpanList _pageLists[NPAGE];
	static PageChche _sInst;
	//std::unordered_map<int, Span*>pageNumberToSpanMap;
	TCMalloc_PageMap1<32 - kPageShift>pageNumberToSpanMap;
	ObjectPool<Span> _objPool;
};