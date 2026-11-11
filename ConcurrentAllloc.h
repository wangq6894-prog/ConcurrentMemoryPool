#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
static void* ConcurrentAlloc(size_t size)
{
    if(size > MAX_BYTES){
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kpage = alignSize >> PAGE_SHIFT;
        Span* span = PageCache::GetInstance()->NewSpan(kpage);
        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        return ptr;
    }else
    {   
        if(pTLSThreadCache == nullptr){
            pTLSThreadCache = new ThreadCache();
        }    
        return pTLSThreadCache->Allocate(size);
    }
}

static void ConcurrentFree(void* ptr,size_t size)
{
    if(size > MAX_BYTES)
    {
        Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();
        return;
    }else{
        assert(pTLSThreadCache);
        pTLSThreadCache->Deallocate(ptr,size);
    }
}
