#include "common.h"
#include "ConcurrentAllot.h"
#include <time.h>
#include <atomic>
#include <stdio.h>

std::unordered_map<size_t, int> Init()
{
	std::unordered_map<size_t, int>tmp;
	int align = 8;
	int c = 0;
	for (int i = align; i <= 1024 * 256; i += align)
	{
		tmp[i] = c;
			align = SizeClass::AlignmentForSize(i);
	}
	return tmp;
}

std::unordered_map<size_t, int> SizeClass::m = Init();



//nWork--->线程数
//nTimes--->每个线程跑几次
//rounds--->每次申请的个数
void BenchmarkMalloc(int nWork, int nTimes, int rounds)
{
	std::vector<std::thread>vthread(nWork);
	std::atomic<size_t> mallocTime = 0;
	for (int k = 0; k < nWork; k++)
	{
		//创建nWork个线程
		vthread[k] = std::thread([&]() {
			std::vector<void*>nums;
			for (int i = 0; i < nTimes; i++)
			{
				nums.reserve(rounds);
				clock_t begin = clock();
				for (int j = 0; j < rounds; j++)
				{
					int sz = (16 + i) % 1024 * 64 + 1;
					nums.push_back(ConcurrentAlloc(sz));
					*(int*)nums[j] = 2;
				}
				for (int j = 0; j < rounds; j++)
				{
					ConcurrentFree(nums[j]);
				}
				clock_t end = clock();
				mallocTime += (end - begin);
				nums.clear();
			}
		});
	}
	for (int i = 0; i < nWork; i++)
		vthread[i].join();
	cout << "BenchmarkMalloc耗时：" << mallocTime << endl;
}

void BenchmarkConcurrentAlloc(int nWork, int nTimes, int rounds)
{
	std::vector<std::thread>vthread(nWork);
	std::atomic<size_t> concurrentAllocTime = 0;
	for (int k = 0; k < nWork; k++)
	{
		//创建nWork个线程
		vthread[k] = std::thread([&]() {
			std::vector<void*>nums;
			for (int i = 0; i < nTimes; i++)
			{
				nums.reserve(rounds);
				clock_t begin = clock();
				for (int j = 0; j < rounds; j++)
				{
					int sz = (16 + i) % 1024 * 64 + 1;
					nums.push_back((malloc(sz)));
					if (nums[j] == nullptr) {
						throw std::bad_alloc();
					}
					*(char*)nums[j] = 'a';
				}

				for (int j = 0; j < rounds; j++)
				{
					free(nums[j]);
				}
				clock_t end = clock();
				concurrentAllocTime += (end - begin);
				nums.clear();
			}
		});
	}
	for (int i = 0; i < nWork; i++)
		vthread[i].join();

	cout << "BenchmarkConcurrentAlloc耗时：" << concurrentAllocTime << endl;
}

int main()
{
	srand((unsigned)time(NULL));
	BenchmarkMalloc(5, 10, 20000);
	BenchmarkConcurrentAlloc(5, 10, 20000);
	return 0;
}
