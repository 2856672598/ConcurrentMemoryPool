#include "common.h"
#include "ConcurrentAllot.h"
#include <set>
#include <thread>
using std::set;

void fun1()
{
	std::vector<void*>nums;
	for (int i = 0; i < 82121; i++)
	{
		nums.push_back(ConcurrentAlloc(1023));
	}
	for (size_t i = 0; i < (size_t)nums.size(); i++)
		ConcurrentFree(nums[i]);
}

void ConcurrentAlloc_Text()
{
	std::thread p1(fun1);
	//std::thread p2(fun1);

	p1.join();
	//p2.join();
	//fun1();
}

//int main()
//{
//	//set<int>s;
//	//set<int>nums;
//	//for (int i = 1; i <= 1024 * 256; i++)
//	//{
//	//	int align = SizeClass::AlignmentForSize(i);
//	//	if (s.insert(align).second)
//	//	{
//	//		//插入成功时输出下i的值
//	//		cout << "size:" << i << " " << align << endl;
//	//	}
//	//	//nums.insert(SizeClass::RoundUp(i));
//	//}
//	//for (auto e : s)
//	//	cout << e << " ";
//	//cout << s.size() << endl;
//
//	//cout << SizeClass::Index(128);
//
//	//cout << SizeClass().Index(128) << endl;
//
//	ConcurrentAlloc_Text();
//	//auto p = new ThreadCache;
//
//	//std::vector<void*>flag;
//	//for (int i = 0; i < 2; i++)
//	//{
//	//	flag.push_back(ConcurrentAlloc(129 * 8 * 1024));
//	//}
//	//for (auto e : flag)
//	//	ConcurrentFree(e, 129 * 8 * 1024);
//
//
//	//ConcurrentAlloc(1024);
//	//ConcurrentAlloc(7);
//	return 0;
//}