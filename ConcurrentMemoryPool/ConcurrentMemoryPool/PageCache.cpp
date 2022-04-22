#include "PageCache.h"

PageChche PageChche::_sInst;

Span* PageChche::NewSpan(size_t n)
{
	if (n > NPAGE - 1) {
		//����128ҳ��ֱ���������
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
	//��nҳ��λ��Ϊ��,��Ҫ������ҷǿ�λ�ý����и�

	for (int i = n; i < NPAGE; i++)
	{
		if (!_pageLists[i].Empty()) {
			//�����и�
			Span* iSpan = _pageLists[i].PopFront();
			Span* span = _objPool.New();
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//���и��ʣ���iSpan �ŵ�ָ��λ��
			_pageLists[iSpan->_npage].PushFront(iSpan);
			//���и��ʣ���iSpan����ӳ��,������кϲ�
			//pageNumberToSpanMap[iSpan->_pagId] = iSpan;
			//pageNumberToSpanMap[iSpan->_pagId + iSpan->count - 1] = iSpan;

			pageNumberToSpanMap.set(iSpan->_pagId, iSpan);
			pageNumberToSpanMap.set(iSpan->_pagId + iSpan->count - 1, iSpan);

			//���ֳ�ȥ��span��ҳ����ӳ�䡣
			for (size_t i = 0; i < span->_npage; i++)
			{
				pageNumberToSpanMap.set(span->_pagId + i, span);
			}

			span->_isUse = true;//����ǰ���״̬����Ϊ��ʹ��
			return span;
		}
	}
	//û���ҵ����ʵ�span�����и�-->ȥϵͳ����
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = _objPool.New();
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_pageLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}

//�ϲ����ݿ�
void PageChche::ReleaseToPageCache(Span* span)
{

	if (span->_npage >= NPAGE) {
		//����NPAGE��ֱ�ӻ�����
		SystemFree((void*)(span->_pagId >> kPageShift));
		_objPool.Delete(span);
		//delete span;
		return;
	}

	//��ǰ�ϲ�
	while (true)
	{
		size_t prevId = span->_pagId - 1;
		Span* prevSpan = (Span*)pageNumberToSpanMap.get(prevId);
		//ǰ���λ�ò�����
		if (prevSpan == nullptr) {
			break;
		}

		//ǰ���span��ʹ��
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
	//���ϲ�
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
		//���Խ��кϲ�
		span->_npage += nextSpan->_npage;
		_objPool.Delete(nextSpan);
		//delete nextSpan;
	}

	//���ϲ��õ�span��ӵ���Ӧ��Ͱ��
	_pageLists[span->_npage].PushFront(span);
	span->_isUse = false;
	//pageNumberToSpanMap[span->_pagId] = span;
	//pageNumberToSpanMap[span->_pagId + span->_npage - 1] = span;

	pageNumberToSpanMap.set(span->_pagId, span);
	pageNumberToSpanMap.set(span->_pagId + span->_npage - 1, span);
}