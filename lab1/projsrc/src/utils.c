#include "utils.h"

void stuffFrame(dArray *a) { 
    u_int8_t byte;

    //skip over first byte which is a flag
    for(int i = 1; i < a->size; i++)
    {
        byte = getArrayValue(&a, i);

        if(byte == FLAG || byte == ESC)
            escapeByte(&a, i, byte);
        
    }
}

void destuffFrame(dArray *a) {
    u_int8_t byte;

    for (int i = 0; i < a->size; i++)
    {   
        byte = getArrayValue(&a, i);

        if (byte == ESC) 
            descapeByte(&a, i);
    }
}