#include "utils.h"

void stuffFrame(dArray *a) { 
    u_int8_t byte;

    for(int i = 0; i < a->size; i++)
    {
        byte = getArrayValue(&a, i);

        if(byte == FLAG || byte == ESC)
            escapeByte(&a, i, ESC);
        
    }
}

void destuffFrame(dArray *a) {

}