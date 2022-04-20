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
			nums.reserve(rounds);
			for (int i = 0; i < nTimes; i++)
			{
				clock_t begin = clock();
				for (int j = 0; j < rounds; j++)
				{
					int sz = (16 + j) % 8192 + 1;
					//int sz = 16;
					nums.push_back(malloc(sz));
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
				mallocTime += (end - begin);
				nums.clear();
			}
		});
	}
	for (int i = 0; i < nWork; i++)
		vthread[i].join();
	cout << nWork << ":个线程，并发：" << nTimes << "次，BenchmarkMalloc耗时：" << mallocTime
		<< "ms,共申请：" << nTimes * rounds << "个空间" << endl;
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
			nums.reserve(rounds);
			for (int i = 0; i < nTimes; i++)
			{
				clock_t begin = clock();
				for (int j = 0; j < rounds; j++)
				{
					int sz = (16 + j) % 8192 + 1;
					//int sz = 16;
					nums.push_back(ConcurrentAlloc(sz));
					*(int*)nums[j] = 2;
				}
				for (int j = 0; j < rounds; j++)
				{
					ConcurrentFree(nums[j]);
				}
				clock_t end = clock();
				concurrentAllocTime += (end - begin);
				nums.clear();
			}
		});
	}
	for (int i = 0; i < nWork; i++)
		vthread[i].join();
	cout << nWork << ":个线程，并发：" << nTimes << "次，BenchmarkConcurrentAlloc耗时：" << concurrentAllocTime 
		<< "ms,共申请：" << nTimes * rounds << "个空间" << endl;

}

int main()
{
	srand((unsigned)time(NULL));
	int n = 10000;
	BenchmarkMalloc(4, 10, n);
	BenchmarkConcurrentAlloc(4, 10, n);
	return 0;
}
