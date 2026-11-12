#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"
static void* ConcurrentAlloc(size_t size)
{
    if(size > MAX_BYTES){
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kpage = alignSize >> PAGE_SHIFT;
        Span* span = PageCache::GetInstance()->NewSpan(kpage);
        span->_objSize = size;
        PageCache::GetInstance()->_pageMtx.unlock();
        
        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        return ptr;
    }else
    {   
        if(pTLSThreadCache == nullptr){
            static ObjectPool<ThreadCache> tcPool;
            pTLSThreadCache = tcPool.New();
        }    
        return pTLSThreadCache->Allocate(size);
    }
}

static void ConcurrentFree(void* ptr,size_t size)
{
    Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
    size_t size = span->_objSize;
    if(size > MAX_BYTES)
    {
        span = PageCache::GetInstance()->MapObjToSpan(ptr);
        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();
        return;
    }else{
        assert(pTLSThreadCache);
        pTLSThreadCache->Deallocate(ptr,size);
    }
}
