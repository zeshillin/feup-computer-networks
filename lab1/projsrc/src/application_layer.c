// Application layer protocol implementation

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>

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
startPacket sPacket;

extern int fd;
extern int cur_seqNum;

int readTLV(unsigned char *packet, TLV *tlv) {
    printf("readingTLV \n");
    tlv->T = packet[0];
    tlv->L = packet[1];
    tlv->V = malloc (tlv->L);
    
    for (int i = 2; i < tlv->L; i++)
        tlv->V[i-2] = packet[i];


    if (tlv->T == T_FILESIZE) {
        file_info.filesize = tlv->L;
    } 
    else if (tlv->T == T_FILENAME) {
        file_info.filename_size = tlv->L;
        file_info.filename = malloc(tlv->L);
        memcpy(file_info.filename, tlv->V, file_info.filename_size);
    }
    else {
        printf("Error receiving TLV info (T parameter undefined). \n");
        return -1;
    }

    return (tlv->L + 2); // return size of V + T and V octects
}

int readControlPacket() {
    printf("readCtrlPacket \n");
    unsigned char packet[BUF_SIZE];
    TLV tlv;
    int ret;
    int index = 1; // start reading packet after the control octet

    printf("before llread\n");
    int bytes = llread(packet);
    if (bytes == -1 || bytes == 0) {
        printf("LLRead failed while trying to read control packet. \n");
        return -1;
    }
    printf("after llread\n");
    if (packet[0] == CF_START) {
        while (index < bytes) {
            if (!(ret = readTLV(packet + index, &tlv)))
                return -1;
            index += ret;
            free(tlv.V);
        }
    }
    else if (packet[0] == CF_END) {
        printf("Finished receiving data. \n");
        return 1;
    }
    else {
        printf("Control packet yielded no discernible control field value. \n");
        return -1;
    }

    return 0;
}
int sendControlPacket(TLV* tlvs, const int tlvNum, u_int8_t cf) {
    printf("%d\n", tlvs[0].L);
    printf("sendCtrlPacket \n");
    int bufSize = 1; // control field
    // calculate full byte length of packet we'll need to send every TLV
    for (int i = 0; i < tlvNum; i++) 
        bufSize += 2 + tlvs[i].L;

    unsigned char *buf = malloc(bufSize); 
    buf[0] = cf;
    int idx;

    printf("loop tlvs \n");
    for (int i = 1; i < tlvNum; i++) {
        buf[idx++] = tlvs[i].T;
        buf[idx++] = tlvs[i].L;
        memcpy(buf + idx, tlvs[i].V, tlvs[i].L);
        idx += tlvs[i].L; 
        printf("%d\n", tlvs[i].L);
    }

    printf("%s\n", buf);
    
    int res = llwrite(buf, idx);
    printf("%d\n", res);
    if (res < 0) {
        free(buf);
        return -1;
    }
    else if (res != idx) {
        printf("Error sending control packet (TLV sizes didn't match llwrite return).\n");
        free(buf);
        return -1;
    }
    printf("here 2\n");
    sPacket.packet_size = bufSize;
    sPacket.packet = malloc(bufSize);
    memcpy(sPacket.packet, buf, bufSize);

    free(buf);
    return res;
}
int sendEndPacket () {
    printf("sendEndPacket \n");
    int res = llwrite(sPacket.packet, sPacket.packet_size);
    if (res < 0) {
        free(sPacket.packet);
        return -1;
    }
    else if (res != sPacket.packet_size) {
        printf("Error sending end packet (TLV sizes didn't match llwrite return).\n");
        free(sPacket.packet);
        return -1;
    }

    free(sPacket.packet);
    return 0;
}

int writeFileContents(FILE *fp) {
    printf("writeFilecnt \n");
    u_int8_t packet[MAX_PACKSIZE] = {0};

    int read_size;
    int data_bytes;
    int res_write;

    while (packet[0] != CF_END) {
        if ((read_size = llread((unsigned char *) packet)) < 0) 
            return -1;
        if (packet[0] != CF_DATA) {
             printf("Error writing to file: read a non-data packet.\n");
            return -1;
        }
        else if (packet[1] != (cur_seqNum++ % 256)) {
            printf("Error writing to file: wrong sequence number frame.\n");
            return -1;
        }

        data_bytes = 256 * packet[2] + packet[3];
        if ((res_write = fwrite(packet + 4, 1, data_bytes, fp)) < 0) {
            printf("Error writing to file: wrong sequence number frame.\n");
            return -1;
        }

        memset(packet, 0, MAX_PACKSIZE);
        
    }

    return 0;
}
int sendFileContents(FILE *fp, u_int8_t size) {
    printf("sendFileCnt \n");
    unsigned char packet[MAX_PACKSIZE];

    int bytes = 0;
    int read_res;
    int write_res;

    printf("The contents of the file are : \n");
    while (bytes < size) {
        
        // insert as much file content into packet as possible (4 bytes will be used for other packet camps) 
        if ((read_res = fread(packet + 4, 1 , MAX_PACKSIZE - 4, fp)) < 0) {
            printf("FRead error while sending file contents. \n");
            return -1;
        }
        packet[0] = CF_DATA;
        packet[1] = (cur_seqNum + 1) % 256; 
        packet[2] = read_res / 256; 
        packet[3] = read_res - packet[2]*256;

        if ((write_res = llwrite(packet, read_res + 4)) < 0)
            return -1;

        bytes += read_res;
        memset(packet, 0, sizeof(packet));

        cur_seqNum++;
    }

    return 0;

}

int readFile() {
    printf("readFile \n");
    // read the starting packet
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

    int res = writeFileContents(fp);
    fclose(fp);

    return res;
}
int sendFile(char* path) {
    printf("sendFile \n");
    TLV filename; // packet structure with filename 
    TLV filesz; // packet structure with filesize

    // get filename & filename size
    const char *fname = basename(path); // use basename() to get a pointer to the base filename
    u_int8_t fname_sz;

    // 256 is the maximum size a file (max of pathname is 4096 in C-lang)
    if ((fname_sz = strnlen(fname, 255)) == 256) {
        printf("Error: Filename is too long.\n"); 
        return -1;
    }

    // get filesize 
    FILE* fp; 
     if ((fp = fopen(path, "r")) == NULL) {
        printf("Error opening file (fopen error). \n");
        return -1;
    }
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    u_int8_t fsize = ftell(fp);
    
    printf("%d \n", fsize);
    fseek(fp, prev, SEEK_SET); 

    // craft filesize TLV
    filesz.T = T_FILESIZE;
    filesz.L = sizeof(fsize);
    filesz.V = malloc(filesz.L);
    memcpy(filesz.V, &fsize, sizeof(fsize));

    printf("%d\n", filesz.L);

    // craft filename TLV
    filename.T = T_FILENAME;
    filename.L = fname_sz;
    filename.V = malloc(fname_sz);
    memcpy(filename.V, fname, fname_sz);

    printf("%d\n", filename.L);
    printf("%s\n", filename.V);

    TLV tlvs[] = {filesz, filename};
    sendControlPacket(tlvs, 2, CF_START); // send start ctrl packet
    
    free(filesz.V);
    free(filename.V);

    int res = sendFileContents(fp, fsize);
    if (res < 0) {
        printf("Error sending file contents.\n");
        fclose(fp);
        return -1;
    }

    sendEndPacket(); // send end ctrl packet

    free(filesz.V);
    free(filename.V);
    fclose(fp);

    return 0;

}

int applicationLayer(const char *serialPort, const char *role, int baudRate,
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
        printf("Role estabilished: Transmissor\n");
        layer.role = LlTx;
    } 
    else {
        printf("Role estabilished: Receiver\n");
        layer.role = LlRx;
    }

    int res = llopen(layer);
    if (res == -1) {
        printf("LLOpen failure: error opening file descriptor.\n");
        return -1;
    }
    else if (res == -2) {
        printf("LLOpen failure (too many tries)\n");
        return -1;
    }
    printf("llopen working");

    return 0;

}

int appLayer_exit() {

    free(file_info.filename);
    if (llclose(0) != 1) {
        return -1;
    }
    return 0;
}