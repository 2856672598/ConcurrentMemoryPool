#pragma once
#include <iostream>
#include <assert.h>
#include <thread>
#include <mutex>

using std::cout;
using std::endl;

static const size_t kAlignment = 8;
static const size_t kMaxSize = 1024 * 256;
static const size_t NFREELIST = 104;//自由链表的个数。
static const size_t NSPANLIST = 1024;


//_WIN64要放在上面，因为在64位环境中既有_WIN32也有_WIN64
#ifdef _WIN64
	typedef  unsigned long long PAGE_ID;
#elif _WIN32
	typedef  size_t PAGE_ID;
#endif



class FreeList
{
public:
	//向自由链表中加入节点
	void Push(void* obj)
	{
		//头插
		*(void**)obj = _head;
		_head = obj;
	}

	void* NextObj(void* obj)
	{
		return *(void**)obj;
	}

	void* Pop()
	{
		if (_head != nullptr){
			//自由链表不为空，直接弹出头节点.
			void* obj = _head;
			_head = NextObj(_head);
			return obj;
		}
		return nullptr;
	}

	bool Empty()
	{
		return  _head == nullptr;
	}
private:
	void* _head = nullptr;
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
private:
	Span* _head;
};