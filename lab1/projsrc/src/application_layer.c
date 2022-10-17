// Application layer protocol implementation

#include <string.h>
#include <stdio.h>

#include "application_layer.h"
#include "link_layer.h"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

extern int fd;
extern int cur_seqNum;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;
    strncpy(layer.serialPort, serialPort, sizeof(layer.serialPort));
    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;
    layer.timeout = timeout;
    printf("checkpoint");
    if ((strcmp(role, "tx")) == 0) {
        printf("role estabilished: tx");
        layer.role = LlTx;
    } 
    else {
        printf("role estabilished: rx");
        layer.role = LlRx;
    }
    int res = llopen(layer);
    if (res == 1)
        printf("llopen working!");
    else if (res == -2)
        printf("too many tries");
    else 
        printf("llopen ended with 0");
    
    llclose(TRUE);


}
