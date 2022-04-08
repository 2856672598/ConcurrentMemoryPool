#include "common.h"
#include "ConcurrentAllot.h"
#include <set>
#include <thread>
using std::set;

//void fun1()
//{
//	for (int i = 0; i < 10; i++)
//	{
//		printf("%d\n", ConcurrentAlloc(65536));
//	}
//}
//
//void ConcurrentAlloc_Text()
//{
//	std::thread p1(fun1);
//	std::thread p2(fun1);
//
//	p1.join();
//	p2.join();
//	//fun1();
//}

int main()
{
	//set<int>s;
	//set<int>nums;
	//for (int i = 1; i <= 1024 * 256; i++)
	//{
	//	int align = SizeClass::AlignmentForSize(i);
	//	if (s.insert(align).second)
	//	{
	//		//插入成功时输出下i的值
	//		cout << "size:" << i << " " << align << endl;
	//	}
	//	//nums.insert(SizeClass::RoundUp(i));
	//}
	//for (auto e : s)
	//	cout << e << " ";
	//cout << s.size() << endl;

	//cout << SizeClass::Index(128);

	//cout << SizeClass().Index(128) << endl;

	//ConcurrentAlloc_Text();
	//auto p = new ThreadCache;
	for (int i = 0; i < 1024; i++)
	{
		ConcurrentAlloc(1020);
	}
	//ConcurrentAlloc(1024);
	//ConcurrentAlloc(7);
	return 0;
}