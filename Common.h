#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <mutex>
#include <assert.h>
#include <algorithm>
#include <new>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif

using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256*1024;
static const size_t NFREELIST = 208;
static const size_t NPAGES = 128;
static const size_t PAGE_SHIFT = 13;

#ifdef _WIN64
    typedef unsigned long long PAGE_ID;
#else
    typedef size_t PAGE_ID;
#endif

// 直接向操作系统申请 kpage 页连续内存
inline void* SystemAlloc(size_t kpage)
{
    void* ptr = nullptr;
#ifdef _WIN32
    ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    // mmap 只保证按系统页(4KB)对齐，而页号按 8KB(PAGE_SHIFT=13) 计算，
    // 因此多申请一页并向上对齐到 8KB，否则 pageId<<PAGE_SHIFT 会偏一页导致越界访问
    size_t bytes = kpage << PAGE_SHIFT;
    size_t align = (size_t)1 << PAGE_SHIFT;
    void* raw = mmap(nullptr, bytes + align, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED)
    {
        ptr = nullptr;
    }
    else
    {
        uintptr_t addr = (uintptr_t)raw;
        ptr = (void*)((addr + align - 1) & ~(uintptr_t)(align - 1));
    }
#endif
    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

inline void*& NextObj(void* obj)
{
    return *(void**)obj;
}
class FreeList
{
    public:
        void Push(void* obj)
        {
            //头插
            //*(void**)obj = _freeList;
            NextObj(obj) = _freeList;
            _freeList = obj;
        }
        void PushRange(void* start,void* end)
        {
            NextObj(end) = _freeList;
            _freeList = start;
        }
        void* Pop()
        {
            //头删
            void* obj = _freeList;
            _freeList = NextObj(obj);
            return obj;
        }
        bool Empty()
        {
            return _freeList == nullptr;
        }
        size_t& MaxSize()
        {
            return _maxSize;
        }
    private:
        void* _freeList = nullptr;
        size_t _maxSize = 1;
};

//计算对象大小的对齐映射规则
class SizeClass
{
    public:
        static inline size_t _RoundUp(size_t size, size_t alignNum)
        {
            //return (size + AlignNum - 1) / AlignNum * AlignNum;
            return ((size + alignNum - 1) & ~(alignNum - 1));
        }
        static inline size_t RoundUp(size_t size)
        {
            assert(size <= MAX_BYTES);
            if (size <= 128){
                return _RoundUp(size, 8);
            }
            else if (size <= 1024){
                return  _RoundUp(size, 16);
            }
            else if (size <= 8*1024){
                return  _RoundUp(size, 128);
            }
            else if (size <= 64*1024){
                return  _RoundUp(size, 1024);
            }
            else if (size <= 256*1024){
                return  _RoundUp(size, 8*1024);
            }
            else{
                assert(false);
                return -1;
            }
        }
        static inline size_t _Index(size_t size, size_t alignNum)
        {
            return ((size + (1 << alignNum) - 1) >> alignNum) - 1;
        }
        static inline size_t Index(size_t size)
        {
            assert(size <= MAX_BYTES);
            static int group_array[4] = { 16, 56, 56, 56 };
            if (size <= 128){
                return _Index(size, 3);
            }
            else if (size <= 1024){
                return  _Index(size - 128, 4) + group_array[0];
            }
            else if (size <= 8*1024){
                return  _Index(size - 1024, 7) + group_array[1] + group_array[0];
            }
            else if (size <= 64*1024){
                return  _Index(size - 8*1024, 10) + group_array[2] + group_array[0] + group_array[1];
            }
            else if (size <= 256*1024){
                return  _Index(size - 64*1024, 13) + group_array[3] + group_array[0] + group_array[1] + group_array[2];
            }
            else{
                assert(false);
                return -1;
            }
        }
    //一次thread cache从中央缓存获取的内存块数量
    //小对象一次申请的多
    //大对象一次申请的少
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);
        // [2, 512]，⼀次批量移动多少个对象的(慢启动)上限值 
        //控制上下限
        int num = MAX_BYTES / size;
        if (num < 2)
            num = 2;
        if (num > 512)
            num = 512;
        return num;
    }
     // 单个对象256KB 
    static size_t NumMovePage(size_t size)
    {
        size_t num = NumMoveSize(size);
        size_t npage = num*size;
        npage >>= PAGE_SHIFT;
        if (npage == 0)
            npage = 1;
        return npage;
    }
};
//管理多个连续页大块内存的跨度结构
struct Span
{
    PAGE_ID _pageId = 0;//大块内存的起始页的页号
    size_t _n = 0;//大块内存的页的数量
    Span* _next = nullptr;//双向链表结构
    Span* _prev = nullptr;
    size_t _useCount = 0;//切好的小块内存，被分配给thread cache的计数
    void* _freeList = nullptr;//切好的小块内存的free list
};
//带头双向链表
class SpanList
{
public:
    SpanList()
    {
        _head = new Span;
        _head->_next = _head;
        _head->_prev = _head;
    };
    Span* Begin()
    {
        return _head->_next;
    }
    Span* End()
    {
        return _head;
    }
    bool Empty()
    {
        return _head->_next == _head;
    }
    void PushFront(Span* span)
    {
        Insert(Begin(),span);
    }
    Span* PopFront()
    {
        Span* front = _head->_next;
        Erase(front);
        return front;
    }
    void Insert(Span* pos,Span* newSpan)
    {
        assert(pos);
        assert(newSpan);

        Span* prev = pos->_prev;
        prev->_next = newSpan;
        newSpan->_prev = prev;
        newSpan->_next = pos;
        pos->_prev = newSpan;
    }
    void Erase(Span* pos)
    {
        assert(pos);
        assert(pos != _head);
        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = next;
        next->_prev = prev;
    }
    ~SpanList()
    {
        Span* cur = _head->_next;
        while (cur != _head)
        {
            Span* next = cur->_next;
            delete cur;
            cur = next;
        }
        delete _head;
    }
private:
    Span* _head;
public:
    std::mutex _mtx;//桶锁
};
