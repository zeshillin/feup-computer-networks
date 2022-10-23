// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "dynamic_array.h"
#include "utils.h"

#include <sys/types.h>

typedef struct {
    char* filename;
    u_int8_t filename_size;
    u_int8_t filesize;
} fileStruct;

typedef struct {
    unsigned char* packet;
    int packet_size;
} startPacket;

typedef struct {
    u_int8_t T;
    u_int8_t L;
    u_int8_t* V;
} TLV;

int readTLV(unsigned char *buf, TLV *tlv);
int readControlPacket();
int sendControlPacket(TLV* tlvs, int tlvNum, u_int8_t cf);

int writeFileContents(FILE *fp);
int sendFileContents(FILE *fp, u_int8_t size);

int readFile();
int sendFile(char* path);

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
int applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

int appLayer_exit();

#endif // _APPLICATION_LAYER_H_
