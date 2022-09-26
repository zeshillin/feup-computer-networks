// Application layer protocol implementation

#include "application_layer.h"

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
    layer.serialPort = serialPort;
    layer.baudrate = baudRate;
    layer.nRetransmissions = nTries;
    layer.timeout = timeout;
    if (role == "tx") {
        layer.LinkLayerRole = llTx;
    } 
    else {
        layer.LinkLayerRole = llRx;
    }
    return llopen(layer);
}
