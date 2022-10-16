#ifndef _DYNAMIC_ARRAY__H_
#define _DYNAMIC_ARRAY__H_

#include <stddef.h>
#include <sys/types.h>

#include "utils.h"

 typedef struct {
  u_int8_t *array;
  size_t used;
  size_t size;
} dArray;

size_t getSize();
u_int8_t getArrayValue(dArray *a, int index);

void initArray(dArray *a, size_t initialSize);
void insertArray(dArray *a, u_int8_t element);
void escapeByte(dArray *a, int index, u_int8_t byte);
void freeArray(dArray *a);

#endif // _DYNAMIC_ARRAY__H_