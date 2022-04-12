#pragma once
#include "common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//TLS�̱߳��ش洢
static _declspec(thread) ThreadCache* TSLThreadCache = nullptr;

static void* ConcurrentAlloc(size_t size)
{
	if (TSLThreadCache == nullptr) {
		static ObjectPool<ThreadCache>pool;

		TSLThreadCache = pool.New();
		//TSLThreadCache = new ThreadCache;
	}

	if (size > kMaxSize) {
		//����256k������
		size_t alignSize = SizeClass::RoundUp(size);
		size_t num_move = alignSize >> kPageShift;
		Span* span = PageChche::GetInstance()->NewSpan(num_move);
		return (void*)(span->_pagId << kPageShift);
	}
	return TSLThreadCache->AllocateMemory(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{
	assert(TSLThreadCache);

	if (size > kMaxSize) {
		Span* span = PageChche::GetInstance()->AddrToSpan(ptr);
		PageChche::GetInstance()->ReleaseToPageCache(span);
		return;
	}

	TSLThreadCache->FreeMemory(ptr, size);
}
