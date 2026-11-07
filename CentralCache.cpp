#pragma once
#include "CentralCache.h"
CentralCache CentralCache::_sInst;
//获取一个非空的span
Span* GetOneSpan(SpanList& list,size_t byte_size)
{
    return nullptr;
}
//从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start,void*& end,size_t batchNum,size_t size)
{
    size_t index = SizeClass::Index(size);
    _spanLists[index].mtx.lock();
    Span* span = GetOneSpan(_spanLists[index],size);
    assert(span);
    assert(span->_freeList);
    // 从span中获取batchNum个对象
    // 如果不够，只获取实际数量的对象
    start = span->_freeList;
    end = start;
    size_t i = 0;
    size_t actualNum = 1;
    while(i < batchNum-1 && NextObj(end) != nullptr)
    {
        end = NextObj(end);
        i++;
        actualNum++;
    }
    span->_freeList = NextObj(end);
    NextObj(end) = nullptr;

    _spanLists[index].mtx.unlock();
    return actualNum;
}