#include "MetaData.h"
#include "PMAllocator.h"
#include "ThreadMemoryPool.h"
#include <libpmem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void *PMA_init(const char *FilePath, size_t size)
{
    //printf("Get into PMA_init()\n");

    /**** 将PM文件映射到虚拟地址空间 ****/
    int IsPM;
    OriginalBaseAddress = pmem_map_file(
        FilePath, size,
        PMEM_FILE_CREATE, 0666,
        &MappedLength, &IsPM);

    if (OriginalBaseAddress == NULL)
    {
        printf("Failed to map the file!\n\n");
        return NULL; // Something wrong
    }
    
    
    /**** 地址空间布局划分 ****/
    BaseAddress =  OriginalBaseAddress;

    flag = BaseAddress;
    BaseAddress += 8;

    FreePagesListHead = BaseAddress;
    BaseAddress += sizeof(struct PageInfo);

    FreePagesListTail = BaseAddress;
    BaseAddress += sizeof(struct PageInfo);

    for(int i=0; i<33; i++){
        PartialPagesHead[i] = BaseAddress;
        BaseAddress += sizeof(struct PageInfo);
    }

    tmp = BaseAddress;
    BaseAddress += sizeof(struct TMP) * MAX_THREAD_NUMBER;

    // 1MB 向上对齐
    if( ((unsigned long long)BaseAddress >> 20) << 20 != (unsigned long long)BaseAddress)
        BaseAddress = (void *)( ( ( (unsigned long long)BaseAddress >> 20) + 1) << 20);


    /**** 初始化 ****/
    FreePagesListHead->PrevPage = FreePagesListTail->NextPage = NULL;
    FreePagesListHead->NextPage = FreePagesListTail->PrevPage = (struct PageInfo *)BaseAddress;

    ((struct PageInfo *)BaseAddress)->PrevPage = FreePagesListHead;
    ((struct PageInfo *)BaseAddress)->NextPage = FreePagesListTail;
    ((struct PageInfo *)BaseAddress)->NumOfFreePages = (OriginalBaseAddress - BaseAddress + MappedLength) / PAGE_SIZE;
    
    printf("OriginalBaseAddress = 0x%016lx\n",(unsigned long)OriginalBaseAddress);
    printf("        BaseAddress = 0x%016lx\n",(unsigned long)BaseAddress);
    printf("       MappedLength = 0x%016lx\n",MappedLength);
    printf("There are %d pages.\n\n",((struct PageInfo *)BaseAddress)->NumOfFreePages);

    for(int i=0; i<SIZE_CLASS_NUMBER; i++)
        PartialPagesHead[i]->NextPage = NULL;
    
    for(int i=0; i<MAX_THREAD_NUMBER; i++){
        tmp[i].PID = 0;
        for(int j=0; j<SIZE_CLASS_NUMBER; j++)
            (tmp[i].PageHead)[j].NextPage = NULL;
    }

    pthread_mutex_init(&TMP_mutex, NULL);
    pthread_mutex_init(&mutex_freePages, NULL);
    for(int i=0; i<SIZE_CLASS_NUMBER; i++)
        pthread_mutex_init(&mutex_partialPages[i], NULL);


    /**** 写入flag ****/
    *flag = MAGIC;

    /*** 持久化 ****/
    pmem_persist(OriginalBaseAddress, BaseAddress-OriginalBaseAddress);

    return BaseAddress;
}


void PMA_exit(void)
{
    *flag = 0;
    pmem_persist(OriginalBaseAddress, 8);
    pmem_unmap(OriginalBaseAddress, MappedLength);
}


void *PMA_malloc(size_t size)
{
    //printf("Get into PMA_malloc()\n");


    // 首先找到申请大小对应的SizeClass下标
    int SCi = FindSCindex(size);

    // 大尺寸分配
    // 直接返回若干个连续的页
    if(SCi == BIG_SIZE_INDEX){
        // 连续页的数量
        int NumOfPages = (size % PAGE_SIZE) ? ((size / PAGE_SIZE) + 1) : (size / PAGE_SIZE) ;
        void *p;
        p = AllocatePages(NumOfPages);
        return p;
    }

    // 小尺寸分配
    // 到自己的内存池中寻找并返回
    struct TMP *MyTMP = FindTMP(pthread_self());
    if(MyTMP == NULL)
        MyTMP = CreateTMP(pthread_self());
    if(MyTMP == NULL){
        printf("Error in CreateTMP()!\n");
        exit(0);
    }

    // 在自己当前持有的页当中寻找
    struct PageInfo *p = (MyTMP->PageHead)[SCi].NextPage;
    while(p != NULL){ 
        if(p->FirstFreeBlock){ // 找到一个有空闲块的页
            void *addr = p->FirstFreeBlock;
            p->FirstFreeBlock = *(void **)addr;
            (p->FreeBlocksNum)--;
            return addr;
        }
        p = p->NextPage;
    }

    // 如果到达这里，说明线程内存池用完了
    // 下面需要对该线程的内存池进行填充

    // 申请一个新页
    struct PageInfo *newPage;
    
    newPage = AllocatePages(1);
    
    // 新页可以拆分成的block的数量，注意要保留头部的页描述符
    int BlocksNum = (PAGE_SIZE - sizeof(struct PageInfo)) / GetSizeByIndex(SCi);
    
    // 第一个block的地址
    void *FirstBlock = (void *)newPage + sizeof(struct PageInfo);

    // 把所有的block串成一个链
    // 每个block的开头存放下一个block的指针
    // 但是这里只串了（BlocksNum-1）个，因为最后一个需要返回给用户来满足本次申请
    for(int i=0; i<BlocksNum-2; i++)
        *(void **)(FirstBlock + i*GetSizeByIndex(SCi)) = FirstBlock + (i+1)*GetSizeByIndex(SCi);
    *(void **)(FirstBlock + (BlocksNum-2)*GetSizeByIndex(SCi)) = NULL;

    //newPage->FreeBlocksNum = BlocksNum-1;
    newPage->FirstFreeBlock = FirstBlock;

    // 将这一新页加到内存池中的链表中去
    newPage->NextPage = (MyTMP->PageHead)[SCi].NextPage;
    (MyTMP->PageHead)[SCi].NextPage = newPage;

    return FirstBlock + (BlocksNum-1)*GetSizeByIndex(SCi);
}

void PMA_free(void *addr, size_t size)
{
    // 首先找到申请大小对应的SizeClass下标
    int SCi = FindSCindex(size);

    // 大尺寸分配
    if(SCi == BIG_SIZE_INDEX){
        // 连续页的数量
        int NumOfPages = (size % PAGE_SIZE) ? ((size / PAGE_SIZE) + 1) : (size / PAGE_SIZE) ;
        FreePages(addr, NumOfPages);
        return ;
    }

    struct PageInfo *p = (struct PageInfo *) (((unsigned long)addr / PAGE_SIZE) * PAGE_SIZE);
    *(void **)addr = p->FirstFreeBlock;
    p->FirstFreeBlock = addr;

}