#pragma once
#include "Common.h"
class CentralCache
{
public:
    static CentralCache* GetInstance()
    {
        return &_sInst;
    };
    //从中心缓存获取一定数量的对象给thread cache
    size_t FetchRangeObj(void*& start,void*& end,size_t batchNum,size_t size);
    //从中心缓存获取一个span给thread cache
    Span* GetOneSpan(SpanList& list,size_t size);
    //将一定数量的对象释放到span跨度
    void ReleaseListToSpans(void* start,size_t byte_size);
private:
    CentralCache(){};
    CentralCache(const CentralCache&) = delete;
    SpanList _spanLists[NFREELIST];
    static CentralCache _sInst;
};
