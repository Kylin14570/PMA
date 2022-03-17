#ifndef METADATA_H
#define METADATA_H

#include "SizeClass.h"
#include "Page.h"
#include "ThreadMemoryPool.h"

#define MAX_THREAD_NUMBER 32  //最多32线程

#define PAGE_SIZE 1048576UL  // 一页为1MB

#define MAGIC 0x2021080799991314
// 分配器初始化完成后，会将MAGIC写入元数据头部的flag
// 分配器正常退出前，会将flag清零
// 所以在初始化时如果检测到flag==MAGIC，说明是一次“脏重启”，需要恢复
// 否则是正常退出，不需要恢复


extern unsigned long long *flag;       // “脏重启”标志

extern struct PageInfo *FreePagesListHead;  //空闲页链表的头指针

extern struct PageInfo *FreePagesListTail;  //空闲页链表的尾指针

extern struct PageInfo *PartialPagesHead[33];   //部分空闲页链表的头指针
// 每一个size class有一个独立的partial页链表

extern struct TMP *tmp; //线程的私有内存池


extern void *OriginalBaseAddress;   //PM文件映射之后的基地址

extern size_t MappedLength;        //PM文件映射的大小

extern void *BaseAddress;          //用户页的起始地址

extern pthread_mutex_t mutex_freePages;
// 申请空闲页面的互斥锁

extern pthread_mutex_t mutex_partialPages[33];
// 申请partial页的互斥锁

extern pthread_mutex_t TMP_mutex;
//访问线程内存池的互斥锁

#endif