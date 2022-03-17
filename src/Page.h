#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>

struct PageInfo{
    struct PageInfo *NextPage; // 下一页的指针
    struct PageInfo *PrevPage; // 上一页的指针
    int NumOfFreePages; //从当前页开始的连续空闲页的个数
    int FreeBlocksNum;     //当前页的空闲块的个数
    void *FirstFreeBlock;  //当前页的第一个空闲块的指针
};

void *AllocatePages(int NumOfPages);
// 分配若干个连续的空闲页

void FreePages(void *FirstPageAddress, int NumOfPages);
// 释放若干个连续的空闲页


#endif