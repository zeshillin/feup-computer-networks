#include "dynamic_array.h"

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

void insertArray(dArray *a, u_int8_t element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2; //might trade for 3/2
    a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  }
  a->array[a->used++] = element;
}

void escapeByte(dArray *a, int index, u_int8_t byte) {

  a->size++;
  a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  a->used++;

  for (int i = index + 1; i < a->size - 1; i++)
    a->array[i+1] = a->array[i];

  a->array[index] = ESC;
  a->array[index+1] = byte^0x20;

}

void descapeByte(dArray *a, int index)
{
  for (int i = index; i < a->size; i++) 
    a->array[i] = a->array[i+1];
  
  a->size--;
  a->array = realloc(a->array, a->size * sizeof(u_int8_t));
  a->used--;
}

void freeArray(dArray *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}
