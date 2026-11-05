#pragma once

#include "common.h"

class ThreadCache
{
    public:
        ThreadCache()
        {
        }
        void* Allocate(size_t size);
        void Deallocate(void* ptr,size_t size);
        ~ThreadCache()
        {
        }
    private:
        FreeList _freeList[NFREE_LIST];
};