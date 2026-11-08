#pragma once

#include "Common.h"

class PageCache
{
public:
    static PageCache* GetInstance()
    {
        return &_sInst;
    }
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
    static PageCache _sInst;
};
