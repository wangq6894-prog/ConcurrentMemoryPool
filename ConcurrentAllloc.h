#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"
static void* ConcurrentAlloc(size_t size)
{
    // 与 malloc(0) 行为对齐：0 字节按 1 字节申请，否则 SizeClass::Index(0) 会下标越界
    if(size == 0) size = 1;
    if(size > MAX_BYTES){
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kpage = alignSize >> PAGE_SHIFT;
        PageCache::GetInstance()->_pageMtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kpage);
        span->_isUse = true;
        span->_objSize = size;
        PageCache::GetInstance()->_pageMtx.unlock();
        
        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        return ptr;
    }else
    {   
        if(pTLSThreadCache == nullptr){
            static ObjectPool<ThreadCache> tcPool;
            // 多个线程首次申请时会同时访问同一个 tcPool，需加锁保护
            static std::mutex tcPoolMtx;
            std::lock_guard<std::mutex> lock(tcPoolMtx);
            pTLSThreadCache = tcPool.New();
        }    
        return pTLSThreadCache->Allocate(size);
    }
}

static void ConcurrentFree(void* ptr,size_t size)
{
    // 同 ConcurrentAlloc：0 字节按 1 字节处理，保证桶下标一致
    if(size == 0) size = 1;
    if(size > MAX_BYTES)
    {
        // 只有大对象才需要查 span，小对象直接还回线程缓存，避免每次都抢 PageCache 的全局锁
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
