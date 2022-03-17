#ifndef SIZECLASS_H
#define SIZECLASS_H

#define BIG_SIZE_INDEX -1
#define SIZE_CLASS_NUMBER 33

#include "unistd.h"

int FindSCindex(size_t size);

unsigned GetSizeByIndex(int index);

#endif