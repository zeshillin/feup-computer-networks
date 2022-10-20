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

fileStruct file_info;
extern int fd;
extern int cur_seqNum;

int readTLV(unsigned char *packet, TLV *tlv) {
    tlv->T = packet[0];
    tlv->L = packet[1];
    tlv->V = malloc (tlv->L);
    
    for (int i = 2; i < tlv->L; i++)
        tlv->V[i-2] = packet[i];

    if (tlv->T == T_FILESIZE) {
        file_info.filesize = tlv->V;
    } 
    else if (tlv->T == T_FILENAME) {
        file_info.filename = malloc(tlv->L);

        file_info.filename_size = tlv->L;
        memcpy(file_info.filename, tlv->V, file_info.filename_size);
    }
    else {
        printf("Error receiving TLV info (T parameter undefined). \n");
        return -1;
    }

    return (tlv->L + 2); // return size of V + T and V octects
}

int readControlPacket() {

    unsigned char* packet[BUF_SIZE];
    TLV tlv;
    int ret;
    int index = 1; // start reading packet after the control octet

    int bytes = llread(&packet);
    if (bytes == -1 || bytes == 0) {
        printf("LLRead failed while trying to read filesize. \n");
        return -1;
    }

    if (packet[0] == CF_START) {
        while (index < bytes) {
            if (!(ret = readTLV(packet + index, &tlv)))
                return -1;
            index += ret;
            free(tlv.V);
        }
    }

    //else handle end control packet

    return 1;
}

int setFile(const char* filename) {
    
    if (!readControlPacket()) {
        printf("Error reading first control packet. \n");
        return -1;
    }

    FILE* fp;

    char filename[file_info.filename_size];
    memcpy(filename, file_info.filename, file_info.filename_size);

    if ((fp = fopen(filename, "w")) == NULL) {
        printf("Error opening file (fopen error). \n");
        return -1;
    }

    int ret = readData(fp);
    fclose(fp);

    return ret;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // helper structures declaration
    LinkLayer layer;

    // link layer setup
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
        printf("llopen ended with 0, program will continue \n");

    // data transfer
    int bytes;

    if (layer.role == LlRx) {
        printf("Receiving file...\n");

        bytes = setFile(filename);
        if  (bytes == 0) {
            printf("File read is empty. \n");
            return 0;
        }
        else if (bytes == -1) {
            printf("File read error. \n");
            return 0;
        }
    }

    
    llclose(layer, 0);
}
