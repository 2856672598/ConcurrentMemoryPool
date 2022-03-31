#pragma once
#include "common.h"
#include "ThreadCache.h"

//TLS线程本地存储
static _declspec(thread) ThreadCache* TSLThreadCache = nullptr;

static void* ConcurrentAlloc(size_t size)
{
	if (TSLThreadCache == nullptr) {
		TSLThreadCache = new ThreadCache;
	}

	return TSLThreadCache->AllocateMemory(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{
	assert(TSLThreadCache);
	TSLThreadCache->FreeMemory(ptr, size);
}
