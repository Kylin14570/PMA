/*****************************************************************
**
**	PMAllocator是一款为持久内存PM设计的内存分配器，以下简称PMA 
**  
**  PMA采用层次化内存管理策略：
**      首先对整体内存进行分页式管理，再对每个页的内部进行小块的管理
**
**  PMA支持多线程并发的内存分配，并为每个线程设置私有内存池
**
**  PMA适用于持久内存PM，分配出去的块是持久性的
**      如果应用程序在正常退出之前没有释放自己申请的块，
**      PMA将保持该块 “in use” 的状态，直到某次运行时该块被释放。
**
**
**
**
*****************************************************************/


#ifndef PMA_H
#define PMA_H

#include <unistd.h>

void *PMA_init(const char *FilePath, size_t size);

void *PMA_malloc(size_t size);

void PMA_free(void *addr, size_t size);

void PMA_exit(void);

#endif 
