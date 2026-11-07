#include "ThreadCache.h"
#include "CentralCache.h"
void* ThreadCache::FetchFromCentralCache(size_t index,size_t size)
{
    //慢增长反馈调节算法
    size_t batchNum = min(_freeLists[index].MaxSize(),SizeClass::NumMoveSize(size));
    void* start = nullptr;
    void* end = nullptr;
    size_t actualNum = CentralCache::Instance()->FetchRangeObj(start,end,batchNum,size);
    assert(actualNum > 1);

    if(actualNum == 1)
    {
        assert(start == end);
        return start;
    }
    else
    {
        _freeLists[index].PushRange(NextObj(start),end);
        return start;
    }
    if(_freeLists[index].MaxSize() == batchNum)
    {
        _freeLists[index].MaxSize() += 1;
    }
    //最开始给的少，后来多次申请给的越来越多
    //第一次要一个给一个，第二次要一个给两个...直到达到上限SizeClass::NumMoveSize(size)
    return nullptr;
}
void* ThreadCache::Allocate(size_t size)
{
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size);//对齐大小
    size_t index = SizeClass::Index(size);//桶索引
    if(!_freeList[index].Empty()){
        return _freeList[index].Pop();
    }else{
        return FetchFromCentralCache(index,alignSize);//线程从中央缓存其独享的内存块
    }
}
void* ThreadCache::Deallocate(void* ptr,size_t size)
{
    assert(ptr);
    assert(size <= MAX_BYTES);

    size_t index = SizeClass::Index(size);
    _freeList[index].Push(ptr);
}