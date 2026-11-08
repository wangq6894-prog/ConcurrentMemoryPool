#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list,size_t size)
{
    //先查看当前的spanlist中是否还有未分配对象的span
    Span* it = list.Begin();
    while(it != list.End())
    {
        if(it->_freeList != nullptr)
        {
            return it;
        }
        else
        {
            it = it -> _next;
        }
    }
    // 先把central cache的桶锁解掉，这样如果其他线程释放内存回来不会阻塞
    list._mtx.unlock();
    // 没有空闲的span了，只能找page cache要
    PageCache::GetInstance()->_pageMtx.lock();
    Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
    PageCache::GetInstance()->_pageMtx.unlock();
    
    //对获取到的span进行切分，不需要加锁，因为其他线程访问不到这个span

    // 计算span的大块内存的起始地址和大块内存的大小(字节数)
    char* start = (char*)(span->_pageId <<PAGE_SHIFT);
    size_t bytes = span->_n << PAGE_SHIFT;
    char* end = start + bytes;
    // 把大块内存切成自由链表接起来
    // 1. 先切一块下来做尾插
    span->_freeList = start;
    start += size;
    void* tail = span->_freeList;
    while(start < end)
    {
        NextObj(tail) = start;
        tail = NextObj(tail);
        start += size;
    }
    NextObj(tail) = nullptr;

    // 切好span后，需要把span插入到central cache的桶list中
    list._mtx.lock();
    list.PushFront(span);

    return span;
}

//从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start,void*& end,size_t batchNum,size_t size)
{
    size_t index = SizeClass::Index(size);
    _spanLists[index]._mtx.lock();
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

    _spanLists[index]._mtx.unlock();
    return actualNum;
}
