#ifndef THP_H
#define THP_H

#include <pthread.h>
#include "Page.h"
#include "SizeClass.h"

// 线程私有内存池
struct TMP{

    // 线程ID
    pthread_t PID;

    //每一种SizeClass的页链表的头节点
    struct PageInfo PageHead[SIZE_CLASS_NUMBER]; 
    
};

// 找到给定线程的内存池的元数据，返回其指针
struct TMP *FindTMP(pthread_t pid);

// 为给定线程创建一个私有内存池，并返回其指针
// 只有在线程第一次调用分配器的时候会触发
struct TMP *CreateTMP(pthread_t pid);


#endif