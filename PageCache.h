#pragma once

#include <unordered_map>
#include "Common.h"
#include "ObjectPool.h"

class PageCache
{
public:
    static PageCache* GetInstance()
    {
        return &_sInst;
    }
    Span* MapObjToSpan(void* obj);
    // 释放空闲span回到Pagecache，并合并相邻的span 
    void ReleaseSpanToPageCache(Span* span);
    // 获取⼀个K⻚的span 
    Span* NewSpan(size_t k);
    std::mutex _pageMtx;
private:
    PageCache()
    {
    }
    ~PageCache()
    {
    }
    SpanList _spanLists[NPAGES];
    ObjectPool<Span> _spanPool;
    std::unordered_map<PAGE_ID,Span*> _idSpanMap;
    static PageCache _sInst;
};
