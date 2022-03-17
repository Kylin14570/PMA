#include "ThreadMemoryPool.h"
#include "MetaData.h"
#include <stdio.h>

// 给定一个线程的线程号
// 找到其私有内存池元数据，返回其指针
struct TMP *FindTMP(pthread_t pid)
{
    // 哈希
    int i = pid % MAX_THREAD_NUMBER;
    int j = i;
    do{ // 线性探测
        // 线程号相等说明找到了
        if(tmp[j].PID == pid)
            return &tmp[j];
        j = (j+1) % MAX_THREAD_NUMBER;
    }while(j!=i);
    // 循环结束说明遍历了一圈还没找到
    return NULL;
}

// 给定一个线程的线程号
// 为其创建一个私有内存池
struct TMP *CreateTMP(pthread_t pid)
{
    // 哈希
    int i = pid % MAX_THREAD_NUMBER;
    
    /**** 进入临界区 ****/
    pthread_mutex_lock(&TMP_mutex);
    
    // 找一个空闲的位置
    while(tmp[i].PID != 0)
        i = (i+1) % MAX_THREAD_NUMBER;
    
    // 占领该位置
    tmp[i].PID = pid;
    
    pthread_mutex_unlock(&TMP_mutex);
    /**** 离开临界区 ****/

    return &tmp[i];
}