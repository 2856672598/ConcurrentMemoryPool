#pragma once
#include "common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//TLS线程本地存储
static _declspec(thread) ThreadCache* TSLThreadCache = nullptr;

static void* ConcurrentAlloc(size_t size)
{
	if (size > kMaxSize) {
		//大于256k的申请
		size_t alignSize = SizeClass::RoundUp(size);
		size_t num_move = alignSize >> kPageShift;

		PageChche::GetInstance()->_mlock.lock();
		Span* span = PageChche::GetInstance()->NewSpan(num_move);
		span->_objSize = size;
		PageChche::GetInstance()->_mlock.unlock();
		return (void*)(span->_pagId << kPageShift);
	}
	if (TSLThreadCache == nullptr) {
		static ObjectPool<ThreadCache>pool;

		TSLThreadCache = pool.New();
	}
	return TSLThreadCache->AllocateMemory(size);
}

static void ConcurrentFree(void* ptr)
{
	assert(TSLThreadCache);

	Span* span = PageChche::GetInstance()->AddrToSpan(ptr);
	size_t size = span->_objSize;

	if (size > kMaxSize) {
		Span* span = PageChche::GetInstance()->AddrToSpan(ptr);

		PageChche::GetInstance()->_mlock.lock();
		PageChche::GetInstance()->ReleaseToPageCache(span);

		PageChche::GetInstance()->_mlock.unlock();
		return;
	}

	TSLThreadCache->FreeMemory(ptr, size);
}
