// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;
    strcpy(layer.serialPort, *serialPort);
    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;
    layer.timeout = timeout;
    if (role == "tx") {
        layer.role = LlTx;
    } 
    else {
        layer.role = LlRx;
    }
    llopen(layer);

    //write if transmissor
    //receive if receptor

}
