#pragma once
#include <iostream>
#include <vector>
#include <assert.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <Windows.h>
using std::cout;
using std::endl;

static const size_t kAlignment = 8;//默认对齐数
static const size_t kMaxSize = 1024 * 256;
static const size_t NFREELIST = 105;//自由链表的个数。
static const size_t NSPANLIST = 105l;
static const size_t NPAGE = 129;
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

	if (ptr == nullptr) {
		throw std::bad_alloc();
	}
	return ptr;
}

inline static void SystemFree(void* ptr)
{
	VirtualFree(ptr, 0, MEM_DECOMMIT);
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
		assert(obj);
		//头插
		NextObj(obj) = _head;
		_head = obj;
		_length++;
	}

	void* Pop()
	{
		assert(_head);
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

	size_t& MaxLength() {
		return _maxLength;
	}
	size_t Length()
	{
		return _length;
	}

	//插入一段范围
	void PushRange(void* start, void* end, int n)
	{
		NextObj(end) = _head;
		_head = start;
		_length += n;
	}

	void PopRange(void*& start, void*& end, size_t n)
	{

		assert(_length >= n);
		start = _head;
		end = start;
		int flag = n - 1;
		while (flag--)
		{
			end = NextObj(end);
		}
		_head = NextObj(end);
		NextObj(end) = nullptr;
		_length -= n;
	}
private:
	void* _head = nullptr;
	size_t _length = 0;//链表的长度
	size_t _maxLength = 1;
};

class SizeClass
{
	SizeClass() = delete;
	SizeClass(const SizeClass&) = delete;
public:
public:
	static  int LgFloor(size_t n) {
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
	/*
	    字节				对齐数		区间内桶的数量
		1--->127			8				16
		128--->255			16				8
		256--->511			32				8
		512--->1023			64				8
		1024--->2047		128				8
		2048--->4095		256				8
		4096--->8191		512				8
		8192--->16383		1024			8
		16384--->32767		2048			8
		32768--->65535		4096			8
		65536--->131071		8192			8
		131072--->262143	16384			8
		262144--->262144	32768
	*/
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
	static size_t Index(size_t s)
	{	
		return m.find(s) != m.end() ? m[s] : -1;
	}
	
	//计算出一次向中心缓存索要的个数。
	static size_t NumMoveSize(size_t bytes)
	{
		if (bytes == 0) return 0;
		int num = 64 * 1024 / bytes;
		if (num < 2)
			num = 2;
		if (num > 32768)
			num = 32768;
		return num;
	}

	//根据申请的大小计算出获取的页数
	static size_t NnmMovePage(size_t bytes)
	{
		size_t number = NumMoveSize(bytes);
		size_t npage = (bytes*number) >> kPageShift;
		if (npage == 0)
			npage = 1;
		return npage;
	}

private:
	static std::unordered_map<size_t, int>m;//字节数和桶下标的映射
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
	bool _isUse = false;//记录当前sapn是否在使用
	size_t _objSize = 0;//切割的每个对象的大小
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

	//pos的后面插入
	void insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);
		Span* next = pos->_next;
		pos->_next = newSpan;
		newSpan->_prev = pos;
		newSpan->_next = next;
		next->_prev = newSpan;
	}

	void Pop(Span* pos)
	{
		assert(pos);
		assert(pos != _head);//不能删头
		Span* prev = pos->_prev;
		prev->_next = pos->_next;

		pos->_next->_prev = prev;
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
		insert(_head, newSpan);
	}

	bool Empty()
	{
		return _head == _head->_next;
	}
	Span* _head;
	std::mutex _mlock;
};