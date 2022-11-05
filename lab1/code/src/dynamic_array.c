#include "dynamic_array.h"

#include <stdlib.h>
#include <stdio.h>

size_t getSize(dArray *a) {
    return a->size;
}

u_int8_t getArrayValue(dArray *a, int index) {
    return a->array[index];
}

void initArray(dArray *a, size_t initialSize) {
  a->array = malloc(initialSize * sizeof(u_int8_t));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(dArray *a, const u_int8_t element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2; //might trade for 3/2
    a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  }
  a->array[a->used++] = element;
}


void stuffFrame(dArray *a) { 
    u_int8_t byte;
    int i = 1;
    int escaped = 0;
    //skip over first byte which is a flag
    for(i = 1; i < a->used; i++) {
        byte = getArrayValue(a, i);
        if(byte == FLAG || byte == ESC) {
          escapeByte(a, i, byte);
          escaped++;
        }
    }
}

void destuffFrame(dArray *a) {
    u_int8_t byte;
    int i = 1;
    int descaped = 0;
    // skip over first and last byte which are flags
    for (i = 1; i < a->used - 1; i++) {   
        byte = getArrayValue(a, i);
        if (byte == ESC) {
            descapeByte(a, i);
            descaped++;
        }
    }
}

void escapeByte(dArray *a, int index, u_int8_t byte) {

  a->size++;
  a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  a->used++;
  for (int i = a->used; i > index + 1; i--) {
    a->array[i] = a->array[i - 1];
  }

  a->array[index] = ESC;
  a->array[index+1] = byte^0x20;
}

void descapeByte(dArray *a, int index)
{
  for (int i = index; i < a->used; i++) {
    a->array[i] = a->array[i+1];
  }
  a->array[index] ^= 0x20;

  a->size--;
  a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  a->used--;
}

u_int8_t generateBCC2(dArray *a) {
  u_int8_t bcc = 0; 

  for (int i = 4; i < a->used - 2; i++)
    bcc ^= a->array[i];

  return bcc;
}

dArray getData(dArray *a) {
  dArray data_array;
  initArray(&data_array, 1);

  for (int i = 4; i < a->used - 2; i++)
    insertArray(&data_array, a->array[i]);
  
  return data_array;
}


void freeArray(dArray *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}
