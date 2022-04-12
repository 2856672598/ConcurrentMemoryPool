#pragma once
#include "common.h"
#include "ObjectPool.h"
class PageChche
{
public:
	static PageChche* GetInstance()
	{
		return &_sInst;
	}

	Span* NewSpan(size_t n);
	std::mutex _mlock;

	Span* AddrToSpan(void* addr)
	{
		//����ַת��Ϊҳ��
		size_t pageId = (size_t)addr >> kPageShift;
		auto ret = pageNumberToSpanMap.find(pageId);
		if (ret != pageNumberToSpanMap.end()) {
			return ret->second;
		}
		//˵��ǰ���߼�������
		assert(false);
		return nullptr;
	}

	void ReleaseToPageCache(Span* span);
private:
	PageChche(const PageChche&) = delete;
	PageChche()
	{}
	SpanList _pageLists[NPAGE];
	static PageChche _sInst;
	std::unordered_map<int, Span*>pageNumberToSpanMap;
	ObjectPool<Span> _objPool;
};