#pragma once

#include "Common.h"

class ThreadCache
{
    public:
        ThreadCache()
        {
        }
        void* Allocate(size_t size);
        void Deallocate(void* ptr,size_t size);
        void* FetchFromCentralCache(size_t index,size_t size);
        void ListToLong(FreeList& list,size_t size);
    private:
        FreeList _freeLists[NFREELIST];
};

// 线程局部存储(TLS)指针：每个线程各自持有一份 pTLSThreadCache，
// 分别指向属于自己的 ThreadCache 对象。
// 这样每个线程只在自己的缓存上分配/释放内存，互不干扰、无需加锁。
// 初始为 nullptr，由该线程首次分配时按需创建。
// inline 保证多个 .cpp 包含本头文件时共享同一个线程局部指针（C++17）
inline thread_local ThreadCache* pTLSThreadCache = nullptr;
//static只在当前文件可见 thread_local 表示线程局部存储，让一个变量「每个线程各有一份独立副本」，而不是所有线程共享同一份
