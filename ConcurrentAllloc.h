#pragma once
#include "common.h"
#include "ThreadCache.h"

void* ThreadCache::FetchFreeList(size_t index,size_t size)
{
    return nullptr;
}

static void* ConcurrentAlloc(size_t size)
{
    if(pTLSThreadCache == nullptr){
        pTLSThreadCache = new ThreadCache();
    }
    return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr)
{
    assert(pTLSThreadCache);
    pThreadCache->Deallocate(ptr);
}