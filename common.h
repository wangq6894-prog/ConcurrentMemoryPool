#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <assert.h>
using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256*1024;
static const size_t NFREE_LIST = 208;

#ifdef _WIN64
    typedef unsigned long long PAGE_ID;
#elif _WIN32
    typedef size_t PAGE_ID;
#else
    //
#endif

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
    private:
        void* _freeList = nullptr;
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

};
//管理多个连续页大块内存的跨度结构
struct Span
{
    PAGE_ID _pageId = 0;//大块内存的起始页的页号
    size_t _size = 0;//页的数量
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
    ~SpanList();
private:
    Span* _head;
    std::mutex _mtx;
};