#include "Page.h"
#include "MetaData.h"
#include <stdio.h>

void *AllocatePages(int NumOfPages)
{
    //printf("Get into AllocatePages()\n");

    pthread_mutex_lock(&mutex_freePages);

    // 遍历空闲页链表，寻找第一个满足大小的空闲段
    struct PageInfo *p = FreePagesListHead->NextPage;
    while(p != FreePagesListTail && p->NumOfFreePages < NumOfPages)
        p = p->NextPage;
    
    // 遍历完了也没找到满足大小的
    if(p == FreePagesListTail){
        pthread_mutex_unlock(&mutex_freePages);
        return NULL;
    }
    
    // 否则说明找到了满足大小要求的，而且即为p所指
    // 但是这仍要分两种情况考虑：
    
    // 1. 当前空闲段的页数恰好与所需页数相等
    // 那么需要将该空闲段从链表中删除
    if(p->NumOfFreePages == NumOfPages){
        struct PageInfo *pp = p->PrevPage;
        struct PageInfo *pn = p->NextPage;
        pp->NextPage = pn;
        pn->PrevPage = pp;
        pthread_mutex_unlock(&mutex_freePages);
        return (void *)p;
    }

    // 2. 当前空闲段的页数多于所需页数
    // 那么只需要从该空闲段的末尾拿出所需页数即可
    // 不需要修改链表
    else{
        void *res = ((char *)p) +  (p->NumOfFreePages - NumOfPages) * PAGE_SIZE;
        p->NumOfFreePages -= NumOfPages;
        pthread_mutex_unlock(&mutex_freePages);
        return res;
    }
}

void FreePages(void *FirstPageAddress, int NumOfPages)
{
    pthread_mutex_lock(&mutex_freePages);

    // 空链表
    if(FreePagesListHead->NextPage == FreePagesListTail){
        FreePagesListHead->NextPage = (struct PageInfo *)FirstPageAddress;
        FreePagesListTail->PrevPage = (struct PageInfo *)FirstPageAddress;
        ((struct PageInfo *)FirstPageAddress)->NextPage = FreePagesListTail;
        ((struct PageInfo *)FirstPageAddress)->PrevPage = FreePagesListHead;
        ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = NumOfPages;
        pthread_mutex_unlock(&mutex_freePages);
        return ;
    }

    // 1. 加在最前面
    if(FirstPageAddress < (void *)FreePagesListHead->NextPage){
        // 如果可以连成一段，用该空闲段替换原来的第一个空闲段，并修改空闲页数量
        if((char *)FirstPageAddress + NumOfPages * PAGE_SIZE == (char *)(FreePagesListHead->NextPage)){
            struct PageInfo *p = FreePagesListHead->NextPage;
            struct PageInfo *pn = p->NextPage;
            ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = p->NumOfFreePages + NumOfPages;
            ((struct PageInfo *)FirstPageAddress)->NextPage = pn;
            ((struct PageInfo *)FirstPageAddress)->PrevPage = FreePagesListHead;
            FreePagesListHead->NextPage = (struct PageInfo *)FirstPageAddress;
            pn->PrevPage = (struct PageInfo *)FirstPageAddress;
        }
        // 否则不能连成一段，需要将该空闲段加入链表中
        else{
            struct PageInfo *p = FreePagesListHead->NextPage;
            ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = NumOfPages;
            ((struct PageInfo *)FirstPageAddress)->NextPage = p;
            ((struct PageInfo *)FirstPageAddress)->PrevPage = FreePagesListHead;
            FreePagesListHead->NextPage = (struct PageInfo *)FirstPageAddress;
            p->PrevPage = (struct PageInfo *)FirstPageAddress;
        }
    }

    // 2. 加在最后面
    else if(FirstPageAddress > (void *)FreePagesListTail->PrevPage){
        // 如果可以连成一段，将该空闲段连到原来最后一个空闲段上即可
        if( ((char *)(FreePagesListTail->PrevPage)) + 
            (FreePagesListTail->PrevPage->NumOfFreePages) * PAGE_SIZE 
            == FirstPageAddress)
        {
            FreePagesListTail->PrevPage->NumOfFreePages += NumOfPages;
        }
        // 否则不能连成一段，需要将该空闲段加入链表中
        else{
            struct PageInfo *p = FreePagesListTail->PrevPage;
            ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = NumOfPages;
            ((struct PageInfo *)FirstPageAddress)->NextPage = FreePagesListTail;
            ((struct PageInfo *)FirstPageAddress)->PrevPage = p;
            p->NextPage = (struct PageInfo *)FirstPageAddress;
            FreePagesListTail->PrevPage = (struct PageInfo *)FirstPageAddress;
        }
    }

    // 3. 加在两个之间
    else{
        struct PageInfo *p1 = FreePagesListHead->NextPage;
        struct PageInfo *p2 = p1->NextPage;
        while(true){
            // 加在p1和p2之间
            if((void *)p1 < FirstPageAddress && FirstPageAddress < (void *)p2)
                break;
            p1 = p2;
            p2 = p2->NextPage;
        }
        //从循环退出以后，该空闲段一定是加在p1和p2之间

        // 是否和前面的空闲段连续
        bool ContinousPrev = ((char *)p1) + p1->NumOfFreePages * PAGE_SIZE == (char *)FirstPageAddress;
        // 是否和后面的空闲段连续
        bool ContinousNext = ((char *)FirstPageAddress) + NumOfPages * PAGE_SIZE == (char *)p2;

        // 1. 前后都不连续
        if( !ContinousPrev && !ContinousNext ){
            ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = NumOfPages;
            ((struct PageInfo *)FirstPageAddress)->NextPage = p2;
            ((struct PageInfo *)FirstPageAddress)->PrevPage = p1;
            p1->NextPage = (struct PageInfo *)FirstPageAddress;
            p2->PrevPage = (struct PageInfo *)FirstPageAddress;
        }
        
        // 2. 只和前面的连续
        else if( ContinousPrev && !ContinousNext ){
            p1->NumOfFreePages += NumOfPages;
        }

        // 3. 只和后面的连续
        else if( !ContinousPrev && ContinousNext ){
            struct PageInfo *p2n = p2->NextPage;
            ((struct PageInfo *)FirstPageAddress)->NumOfFreePages = NumOfPages + p2->NumOfFreePages;
            ((struct PageInfo *)FirstPageAddress)->NextPage = p2n;
            ((struct PageInfo *)FirstPageAddress)->PrevPage = p1;
            p1->NextPage = (struct PageInfo *)FirstPageAddress;
            p2n->PrevPage = (struct PageInfo *)FirstPageAddress;
        }

        // 4. 前后都连续
        else{
            struct PageInfo *p2n = p2->NextPage;
            p1->NumOfFreePages += NumOfPages + p2->NumOfFreePages;
            p1->NextPage = p2n;
            p2n->PrevPage = p1;
        }
    }
    pthread_mutex_unlock(&mutex_freePages);
}