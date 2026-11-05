#include "ThreadCache.h"

void* ThreadCache::Allocate(size_t size)
{
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size);//对齐大小
    size_t index = SizeClass::Index(size);//桶索引
    if(!_freeList[index].Empty()){
        return _freeList[index].Pop();
    }else{
        return FetchFromCentralCache(index,alignSize);
    }
}
void* ThreadCache::Deallocate(void* ptr,size_t size)
{
    assert(ptr);
    assert(size <= MAX_BYTES);

    size_t index = SizeClass::Index(size);
    _freeList[index].Push(ptr);
}