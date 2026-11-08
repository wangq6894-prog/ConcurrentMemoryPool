#pragma once
#include "Common.h"
class CentralCache
{
public:
    static CentralCache* GetInstance()
    {
        return &_sInst;
    };
    size_t FetchRangeObj(void*& start,void*& end,size_t batchNum,size_t size);
    Span* GetOneSpan(SpanList& list,size_t size);
private:
    CentralCache(){};
    CentralCache(const CentralCache&) = delete;
    SpanList _spanLists[NFREELIST];
    static CentralCache _sInst;
};
