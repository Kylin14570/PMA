#include "PMAllocator.h"
#include "Page.h"
#include "MetaData.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define ITR 1000
#define N 10000



void *test(void *arg)
{
    void *p[N];
    for(int itr=0; itr<ITR; itr++){
        for(int i=0; i<N; i++)
            p[i] = PMA_malloc(16);
        for(int i=0; i<N; i++)
            PMA_free(p[i],16);    
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    char *FilePath = argv[1];
    size_t len = (unsigned long)atoi(argv[2]) * 1024 * 1024 * 1024;
    int T = atoi(argv[3]);

    struct PageInfo *addr = PMA_init(FilePath, len);
    
    clock_t start = clock();

    pthread_t tid[MAX_THREAD_NUMBER];

    for(int i = 0; i < T; i++)
        pthread_create( &tid[i], NULL, test, NULL);
    
    for(int i = 0; i < T; i++)
        pthread_join( tid[i], NULL);

    clock_t end = clock();

    printf("Time = %ldms\n\n", (end-start)*1000/CLOCKS_PER_SEC);
    
    PMA_exit();
    
    printf("Exit normally.\n");
 
    return 0;
}