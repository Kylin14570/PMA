#include "MetaData.h"

unsigned long long *flag;       // “脏重启”标志

struct PageInfo *FreePagesListHead;  //空闲页链表的头指针

struct PageInfo *FreePagesListTail;  //空闲页链表的尾指针

struct PageInfo *PartialPagesHead[33];   //部分空闲页链表的头指针
// 每一个size class有一个独立的partial页链表

struct TMP *tmp; //线程的私有内存池


void *OriginalBaseAddress;   //PM文件映射之后的基地址

size_t MappedLength;        //PM文件映射的大小

void *BaseAddress;          //用户页的起始地址

pthread_mutex_t mutex_freePages;
// 申请空闲页面的互斥锁

pthread_mutex_t mutex_partialPages[33];
// 申请partial页的互斥锁

pthread_mutex_t TMP_mutex;
//访问线程内存池的互斥锁

/********************************************************************************
 
 *      PM文件映射到内存空间布局如下：
 * 
 *                       _______________________
 *                      |                       |
 *                      |          flag         |
 *                      |_______________________|
 *                      |                       |
 *                      |   FreePagesListHead   |
 *                      |_______________________|
 *                      |                       |
 *                      |   FreePagesListTail   |
 *                      |_______________________|
 *                      |                       |
 *                      |  PartialPagesHead[0]  |
 *                      |_______________________|
 *                      |                       |
 *                      |  PartialPagesHead[1]  |
 *                      |_______________________|
 *                      |                       |
 *                      |  PartialPagesHead[2]  |
 *                      |_______________________|
 *                      |                       |
 *                      | PartialPagesHead[...] |
 *                      |_______________________|
 *                      |                       |
 *                      |        tmp[0]         |
 *                      |_______________________|
 *                      |                       |
 *                      |        tmp[1]         |
 *                      |_______________________|
 *                      |                       |
 *                      |        tmp[2]         |
 *                      |_______________________|
 *                      |                       |
 *                      |       tmp[...]        |
 *                      |_______________________| 
 *                      |                       |
 *                      |                       |
 *                      |                       |
 *                      |      user space       |
 *                      |                       |
 *                      |                       |
 *                      |                       |
 *                      |_______________________|
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * *****************************************************************************/