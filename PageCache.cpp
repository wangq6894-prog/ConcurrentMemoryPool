#include "PageCache.h"

PageCache PageCache::_sInst;

//获取一个K页的span
Span* PageCache::NewSpan(size_t k)
{
    assert(k > 0 && k <NPAGES);
    // 先检查第K页是否有span
    if(!_spanLists[k].Empty())
    {
        return _spanLists[k].PopFront();
    }
    //检查一下后面的桶里面有没有span，如果有可以把它进行切分
    for(size_t i = k+1;i < NPAGES;i++)
    {
        if(!_spanLists[i].Empty())
        {
            Span* nSpan = _spanLists[i].PopFront();
            Span* kSpan = new Span;
            // 在nSpan的头部切k页下来
            // k页返回
            // n页继续切分
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_n = k;

            nSpan->_pageId += k;
            nSpan->_n -= k;

            _spanLists[nSpan->_n].PushFront(nSpan);
            return kSpan;
        }
    }
    
    //走到这个位置就说明后边没有更大页的span了
    //这时候就要去堆要一个128页的span
    Span* bigSpan = new Span;
    void* ptr = SystemAlloc(NPAGES - 1);
    bigSpan -> _pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
    bigSpan -> _n = NPAGES - 1;
    _spanLists[bigSpan->_n].PushFront(bigSpan);
    return NewSpan(k);
}
