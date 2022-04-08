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
	//��nҳ��λ��Ϊ��,��Ҫ������ҷǿ�λ�ý����и�

	for (int i = n; i < NPAGE; i++)
	{
		if (!_pageLists[i].Empty()) {
			//�����и�
			Span* iSpan = _pageLists[i].PopFront();
			Span* span = new Span;
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//���и��ʣ���iSpan �ŵ�ָ��λ��
			_pageLists[iSpan->_npage].PushFront(iSpan);
			//���и��ʣ���iSpan����ӳ��,������кϲ�
			pageNumberToSpanMap[iSpan->_pagId] = iSpan;
			pageNumberToSpanMap[iSpan->_pagId + iSpan->count - 1] = iSpan;

			//���ֳ�ȥ��span��ҳ����ӳ�䡣
			for (size_t i = 0; i < span->_npage; i++)
			{
				pageNumberToSpanMap[span->_pagId + i] = span;
			}

			return span;
		}
	}
	cout << "����" << n << "ҳ�ռ�" << endl;
	//û���ҵ����ʵ�span�����и�-->ȥϵͳ����
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = new Span;
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_pageLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}