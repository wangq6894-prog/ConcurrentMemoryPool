#pragma once
#include <iostream>
#include <vector>
#include <time.h>
#include <new>
#include <stdexcept>
#include "Common.h"
using std::cout;
using std::endl;

// 定长内存池
template <typename T>
class ObjectPool
{
public:
    T *New()
    {
        T *obj = nullptr;
        // 优先利用环回内存块
        if (_freeList != nullptr)
        {
            void *next = *(void **)_freeList;
            obj = (T *)_freeList;
            _freeList = next;
            return obj;
        }
        else
        {
            if (_remainBytes < sizeof(T))
            {
                //_memory = (char *)malloc(128 * 1024);
                _remainBytes = 128 * 1024; // 目标块大小 128KB
                _memory = (char *)SystemAlloc(_remainBytes / (1 << PAGE_SHIFT));
                if (_memory == nullptr)
                {
                    throw std::bad_alloc();
                }
            }
            obj = (T *)_memory;
            size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
            _memory += objSize;
            _remainBytes -= objSize;
        }
        // 定位new
        new (obj) T;
        return obj;
    }
    void Delete(T *obj)
    {
        obj->~T();
        // 还回来一个对象
        if (_freeList == nullptr)
        {
            _freeList = obj;
            *((void **)obj) = nullptr; // void* 4字节 void* 8字节  只要是二级指针就行
        }
        else
        {
            *((void **)obj) = _freeList;
            _freeList = obj;
        }
    }

private:
    char *_memory = nullptr;   // 指向大块内存的指针
    size_t _remainBytes = 0;   // 记录大块内存在切分过程中的剩余字节数
    void *_freeList = nullptr; // 还回来过程中链接的自由链表头指针
};
struct TreeNode
{
    int _val;
    TreeNode *_left;
    TreeNode *_right;

    TreeNode()
        : _val(0), _left(nullptr), _right(nullptr)
    {
    }
};

inline void TestObjectPool()
{
    // 申请释放的轮次
    const size_t Rounds = 5;

    // 每轮申请释放多少次
    const size_t N = 100000;

    std::vector<TreeNode *> v1;
    v1.reserve(N);

    size_t begin1 = clock();
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
        }
        for (int i = 0; i < N; ++i)
        {
            delete v1[i];
        }
        v1.clear();
    }

    size_t end1 = clock();

    std::vector<TreeNode *> v2;
    v2.reserve(N);

    ObjectPool<TreeNode> TNPool;
    size_t begin2 = clock();
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v2.push_back(TNPool.New());
        }
        for (int i = 0; i < N; ++i)
        {
            TNPool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();

    cout << "new cost time:" << end1 - begin1 << endl;
    cout << "object pool cost time:" << end2 - begin2 << endl;
}