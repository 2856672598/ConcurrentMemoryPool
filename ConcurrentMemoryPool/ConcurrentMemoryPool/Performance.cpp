#include "common.h"
#include "ConcurrentAllot.h"

void Fun(int nTimes, int rounds)
{
	std::vector<void*>nums;
	for (int i = 0; i < rounds; i++)
	{
		for (int j = 0; j < nTimes; j++)
		{
			nums.push_back(ConcurrentAlloc(128));
			void* p = nums[j];
			*(int*)p = 2;
		}
		for (int j = 0; j < nTimes; j++)
		{
			ConcurrentFree(nums[j]);
		}
		nums.clear();
	}
}

void BenchmarkMalloc(int nTimes, int nWork, int rounds)
{
	std::vector<std::thread>vthread(nWork);
	for (int i = 0; i < nWork; i++)
	{
		//创建nWork个线程
		vthread[i] = std::thread(Fun, nTimes, rounds);
	}
	for (int i = 0; i < nWork; i++)
		vthread[i].join();
}
int main()
{
	BenchmarkMalloc(34245, 3, 3);
	return 0;
}