#pragma once
#include <iostream>
#include <assert.h>
#include <thread>
#include <mutex>
#include <algorithm>
#include <Windows.h>
using std::cout;
using std::endl;

static const size_t kAlignment = 8;
static const size_t kMaxSize = 1024 * 256;
static const size_t NFREELIST = 104;//��������ĸ�����
static const size_t NSPANLIST = 1024;
static const size_t NPAGE = 128;
static const size_t kPageShift = 13;


//_WIN64Ҫ�������棬��Ϊ��64λ�����м���_WIN32Ҳ��_WIN64
#ifdef _WIN64
	typedef  unsigned long long PAGE_ID;
#elif _WIN32
	typedef  size_t PAGE_ID;
#endif


	// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap��
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}


static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

class FreeList
{
public:
	//�����������м���ڵ�
	void Push(void* obj)
	{
		//ͷ��
		*(void**)obj = _head;
		_head = obj;
		_length++;
	}

	void* Pop()
	{
		if (_head != nullptr){
			//��������Ϊ�գ�ֱ�ӵ���ͷ�ڵ�.
			void* obj = _head;
			_head = NextObj(_head);
			_length--;
			return obj;
		}
		return nullptr;
	}

	bool Empty()
	{
		return  _head == nullptr;
	}

	size_t length() const {
		return _length;
	}

	//����һ�η�Χ
	void PushRange(void* start, void* end, int n)
	{
		NextObj(end) = _head;
		_head = start;
		_length += n;
	}
private:
	void* _head = nullptr;
	size_t _length = 1;//����ĳ���
};

class SizeClass
{
public:
	SizeClass()
	{
		int align = 8;
		int c = 0;
		for (int i = align; i <= 1024 * 256; i += align)
		{
			index_arr[c++] = i;
			if (align != AlignmentForSize(i))
				align = AlignmentForSize(i);
		}
	}
public:
	static inline int LgFloor(size_t n) {
		int log = 0;
		for (int i = 4; i >= 0; --i) {
			int shift = (1 << i);
			size_t x = n >> shift;
			if (x != 0) {
				n = x;
				log += shift;
			}
		}
		assert(n == 1);
		return log;
	}

	//����ָ����С�Ķ�����
	static inline int AlignmentForSize(size_t size)
	{
		int alignment = kAlignment;
		size_t kPageSize = 1 << 13;
		if (size > kMaxSize) {
			// ���ڴ�ߴ磬�� kPageSize ���������ޡ�
			alignment = kPageSize;
		}
		else if (size >= 128) {
			alignment = (1 << LgFloor(size)) / 8;
		}
		//else if (size >= 16) {
		//	//16 - 128�ֽڰ��� 16����
		//	alignment = 16;
		//}
		//if (alignment > kPageSize) {
		//	alignment = kPageSize;
		//}
		return alignment;
	}

	//���϶���
	static inline size_t RoundUp(size_t bytes)
	{
		int alignNum = AlignmentForSize(bytes);
		//if (bytes%alignNum == 0)
		//	return bytes;
		//return (bytes / alignNum + 1)*alignNum;

		return ((bytes + alignNum - 1)&~(alignNum - 1));
	}

	//Ͱ���±�
	size_t Index(size_t s)
	{
		for (int i = 0; i < NFREELIST; i++)
		{
			if (index_arr[i] == s)
				return i;
		}
		return -1;
	}
	
	//�����һ�������Ļ�����Ҫ�ĸ�����
	static int NumMoveSize(size_t bytes)
	{
		if (bytes == 0) return 0;
		// Use approx 64k transfers between thread and central caches.
		int num = static_cast<int>(64 * 1024.0 / bytes);
		if (num < 2)
			num = 2;
		//32768
		if (num > 32768)
			num = 32768;
		return num;
	}

	//��������Ĵ�С�������ȡ��ҳ��
	static size_t MnmMovePage(size_t bytes)
	{
		size_t number = NumMoveSize(bytes);
		size_t npage = (bytes*number) >> kPageShift;
		if (npage == 0)
			npage = 1;
		return npage;
	}

private:
	int index_arr[NFREELIST];
};


class Span
{
public:
	Span* _next = nullptr;  //��ͷ˫��ѭ������
	Span* _prev = nullptr;
	size_t _npage = 0; //ҳ��
	PAGE_ID _pagId = 0; //ҳ��
	void* _freeList = nullptr; //��������
	size_t count = 0;//��¼�������
	size_t bytes = 0;
};

class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Push(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);
		newSpan->_next = pos->_next;
		pos->_next = newSpan;
		newSpan->_prev = pos;
		newSpan->_next->_prev = newSpan;
	}

	void Pop(Span* pos)
	{
		assert(pos);
		assert(pos != _head);//����ɾͷ
		Span* prev = pos->_prev;
		prev->_next = pos->_next;
		pos->_prev = prev;
	}

	Span* PopFront()
	{
		assert(!Empty());
		Span* top = _head->_next;
		Pop(top);
		return top;
	}

	//ͷ��
	void PushFront(Span* newSpan)
	{
		Push(_head, newSpan);
	}

	bool Empty()
	{
		return _head == _head->_next;
	}
	Span* _head;
	std::mutex _mlock;
};