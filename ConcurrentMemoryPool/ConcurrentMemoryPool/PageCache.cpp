#include "PageCache.h"

PageChche PageChche::_sInst;

Span* PageChche::NewSpan(size_t n)
{
	cout << "����" << n << "ҳ�ռ�" << endl;
	if (!_spanLists[n].Empty()) {
		return _spanLists[n].PopFront();
	}
	//��nҳ��λ��Ϊ��,��Ҫ������ҷǿ�λ�ý����и�

	for (int i = n; i < NPAGE; i++)
	{
		if (!_spanLists[i].Empty()) {
			//�����и�
			Span* iSpan = _spanLists[i].PopFront();
			Span* span = new Span;
			span->_pagId = iSpan->_pagId;
			span->_npage = n;

			iSpan->_pagId += n;
			iSpan->_npage -= n;

			//���и���span �ŵ�ָ��λ��
			_spanLists[iSpan->_npage].PushFront(iSpan);
			return span;
		}
	}

	//û���ҵ����ʵ�span�����и�-->ȥϵͳ����
	void* ptr = SystemAlloc(NPAGE - 1);
	Span* bigSpan = new Span;
	bigSpan->_pagId = (unsigned)ptr >> kPageShift;
	bigSpan->_npage = NPAGE - 1;
	_spanLists[NPAGE - 1].PushFront(bigSpan);

	return NewSpan(n);
}