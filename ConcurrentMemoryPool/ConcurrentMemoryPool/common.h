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
static const size_t NFREELIST = 104;//自由链表的个数。
static const size_t NSPANLIST = 1024;
static const size_t NPAGE = 128;
static const size_t kPageShift = 13;


//_WIN64要放在上面，因为在64位环境中既有_WIN32也有_WIN64
#ifdef _WIN64
	typedef  unsigned long long PAGE_ID;
#elif _WIN32
	typedef  size_t PAGE_ID;
#endif


	// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等
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
	//向自由链表中加入节点
	void Push(void* obj)
	{
		//头插
		*(void**)obj = _head;
		_head = obj;
		_length++;
	}

	void* Pop()
	{
		if (_head != nullptr){
			//自由链表不为空，直接弹出头节点.
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

	//插入一段范围
	void PushRange(void* start, void* end, int n)
	{
		NextObj(end) = _head;
		_head = start;
		_length += n;
	}
private:
	void* _head = nullptr;
	size_t _length = 1;//链表的长度
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

	//返回指定大小的对齐数
	static inline int AlignmentForSize(size_t size)
	{
		int alignment = kAlignment;
		size_t kPageSize = 1 << 13;
		if (size > kMaxSize) {
			// 对于大尺寸，在 kPageSize 处对齐上限。
			alignment = kPageSize;
		}
		else if (size >= 128) {
			alignment = (1 << LgFloor(size)) / 8;
		}
		//else if (size >= 16) {
		//	//16 - 128字节按照 16对齐
		//	alignment = 16;
		//}
		//if (alignment > kPageSize) {
		//	alignment = kPageSize;
		//}
		return alignment;
	}

	//向上对齐
	static inline size_t RoundUp(size_t bytes)
	{
		int alignNum = AlignmentForSize(bytes);
		//if (bytes%alignNum == 0)
		//	return bytes;
		//return (bytes / alignNum + 1)*alignNum;

		return ((bytes + alignNum - 1)&~(alignNum - 1));
	}

	//桶的下标
	size_t Index(size_t s)
	{
		for (int i = 0; i < NFREELIST; i++)
		{
			if (index_arr[i] == s)
				return i;
		}
		return -1;
	}
	
	//计算出一次向中心缓存索要的个数。
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

	//根据申请的大小计算出获取的页数
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
	Span* _next = nullptr;  //带头双向循环链表
	Span* _prev = nullptr;
	size_t _npage = 0; //页数
	PAGE_ID _pagId = 0; //页号
	void* _freeList = nullptr; //自由链表
	size_t count = 0;//记录分配情况
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
		assert(pos != _head);//不能删头
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

	//头插
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