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

extern int fd;

fileStruct file_info;
startPacket sPacket;
int cur_seqNum = 0;

int readTLV(unsigned char *packet, TLV *tlv) {
    tlv->T = packet[0];
    tlv->L = packet[1];
    tlv->V = malloc (tlv->L);
    
    for (int i = 2; i < tlv->L + 2; i++)
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
    unsigned char packet[BUF_SIZE];
    TLV tlv;
    int ret;
    int index = 1; // start reading packet after the control octet

    int bytes = llread(packet);
    if (bytes == -1 || bytes == 0) {
        printf("LLRead failed while trying to read control packet. \n");
        return -1;
    }

    if (packet[0] == CF_START) {
        while (index < bytes) {
            ret = readTLV(packet + index, &tlv);
            index += ret;
            free(tlv.V);
        }
    }
    else if (packet[0] == CF_END) {
        printf("Finished receiving data. \n");
        return 1;
    }
    else {
        printf("Control packet yielded no discernible control field value: %c \n", packet[0]);
        return -1;
    }

    return 0;
}
int sendControlPacket(TLV* tlvs, const int tlvNum, u_int8_t cf) {

    int packSize = 1; // control field
    // calculate full byte length of packet we'll need to send every TLV
    for (int i = 0; i < tlvNum; i++) 
        packSize += 2 + tlvs[i].L;

    unsigned char *packet = malloc(packSize); 
    packet[0] = cf; 
    int idx = 1;

    for (int i = 0; i < tlvNum; i++) {
        packet[idx++] = tlvs[i].T;
        packet[idx++] = tlvs[i].L;
        memcpy(packet + idx, tlvs[i].V, tlvs[i].L);
        idx += tlvs[i].L; 
    }

    // write control packet
    int res = llwrite(packet, idx);
    if (res < 0) {
        printf("Error sending control packet: control packet was empty.\n");
        free(packet);
        return -1;
    }
    else if (res < idx) {
        printf("Error sending control packet: TLV sizes didn't match llwrite return.\n");
        free(packet);
        return -1;   
    }
    
    sPacket.packet_size = packSize;
    sPacket.packet = malloc(packSize);
    memcpy(sPacket.packet, packet, packSize);
    
    free(packet);
    return res;
}
int sendEndPacket () {
    unsigned char *packet = malloc(sPacket.packet_size); 
    memcpy(packet, sPacket.packet, sPacket.packet_size);
    packet[0] = CF_END;

    int res = llwrite(packet, sPacket.packet_size);
    if (res < 0) {
        return -1;
    }
    else if (res < sPacket.packet_size) {
        printf("Error sending end packet (TLV sizes didn't match llwrite return).\n");
        return -1;
    }
    free(packet);
    return 0;
}

int writeFileContents(FILE *fp) {
    u_int8_t* packet = malloc(MAX_PACKSIZE);

    int read_size;
    int data_bytes;
    int res_write;

    while (1) {
        
        if ((read_size = llread(packet)) < 0) 
            continue;
        else if (packet[0] == CF_END) {
            break;
        }
        else if (packet[0] != CF_DATA) {
            printf("Error writing to file: read a non-data packet: %x\n", packet[0]);
            continue;
        }
        else if (packet[1] != ((cur_seqNum++) % 256)) {
            printf("Error writing to file: wrong sequence number frame.\n");
            return -1;
        }
 
        else {
            data_bytes = 256 * packet[2] + packet[3];
            if ((res_write = fwrite(packet + 4, 1, data_bytes, fp)) < 0) {
                printf("Error writing to file: fwrite returned an error.\n");
                return -1;
            }
        }

        memset(packet, 0, MAX_PACKSIZE);     
    }

    printf("Finished file.\n");
    appLayer_exit();
    return 0;
}

int sendFileContents(FILE *fp, long size) {
    unsigned char packet[MAX_PACKSIZE];

    long file_to_go = size;
    int read_res;
    int content_size = MAX_PACKSIZE - 4;

    while (file_to_go > 0) {
        //printf("file to go: %ld\n", file_to_go);
        if (file_to_go < MAX_PACKSIZE - 4) {
            content_size = file_to_go;
        }

        // insert as much file content into packet as possible (4 bytes will be used for other packet fields) 
        if ((read_res = fread(packet + 4, 1 , content_size, fp)) < 0) {
            printf("FRead error while sending file contents. \n");
            return -1;
        }

        packet[0] = CF_DATA;
        packet[1] = (unsigned char) ((cur_seqNum++) % 256); 
        packet[2] = (unsigned char) (read_res / 256); 
        packet[3] = (unsigned char) (read_res % 256);

        if ((llwrite(packet, read_res + 4)) < 0) {
            memset(packet, 0, sizeof(packet));
            return -1;
        }
        else {
            file_to_go -= read_res;
            memset(packet, 0, sizeof(packet));
        }

    }

    return 0;

}

int readFile() {
    // read the starting packet
    if (readControlPacket() < 0) {
        printf("Error reading first control packet. \n");
        return -1;
    }

    // craft new filename 
    char* new_filename = malloc(file_info.filename_size + 9);

    char* token = strtok(file_info.filename, ".");
    strcpy(new_filename, token);
    strcat(new_filename, "-received.");
    strcat(new_filename, file_info.filename + (strlen(token) + 1));
    
    FILE* fp;
    if ((fp = fopen(new_filename, "w")) == NULL) {
        printf("Error opening file (fopen error). \n");
        return -1;
    }
    free(new_filename);

    // write to file received packets sent by transmitter
    int res = writeFileContents(fp);

    fclose(fp);
    return res;
}
int sendFile(char* path) {
    TLV filename; // packet structure with filename 
    TLV filesz; // packet structure with filesize

    // get filename & filename size
    const char *fname = basename(path); // use basename() to get a pointer to the base filename
    u_int8_t fname_sz;
    // 256 is the maximum size a file (max of pathname is 4096 in C-lang)
    if ((fname_sz = strnlen(fname, 256)) == 256) {
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
    long fsize = ftell(fp);
    fseek(fp, prev, SEEK_SET); 

    // craft filesize TLV
    filesz.T = T_FILESIZE;
    filesz.L = sizeof(fsize);
    filesz.V = malloc(filesz.L*sizeof(unsigned char));
    memcpy(filesz.V, &fsize, sizeof(fsize));

    // craft filename TLV
    filename.T = T_FILENAME;
    filename.L = fname_sz;
    filename.V = malloc(fname_sz);
    memcpy(filename.V, fname, fname_sz);

    // send TLVs in starting packet
    TLV tlvs[] = {filesz, filename};
    if (sendControlPacket(tlvs, 2, CF_START) < 0) 
        return -1; 
    free(filesz.V);
    free(filename.V);

    // send file contents
    int res = sendFileContents(fp, fsize);
    if (res < 0) {
        printf("Error sending file contents.\n");
        fclose(fp);
        return -1;
    }

    int end_res;
    if ((end_res = sendEndPacket()) < 0) {// send end ctrl packet
        return -1;
    }

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
    
    if ((strcmp(role, "tx")) == 0) {
        printf("Role estabilished: Transmitter\n\n");
        layer.role = LlTx;
    } 
    else {
        printf("Role estabilished: Receiver\n\n");
        layer.role = LlRx;
    }

    printf("Estabilishing connection with llopen...\n\n");
    int res = llopen(layer);
    if (res == -1) {
        printf("LLOpen failure: error opening file descriptor.\n\n");
        return -1;
    }
    else if (res == -2) {
        printf("LLOpen failure: too many tries).\n\n");
        return -1;
    }
    printf("Connection estabilished!\n\n");

    cur_seqNum = 0;

    return 0;

}

int appLayer_exit() {

    free(file_info.filename);
    free(sPacket.packet);

    printf("Ending connection with llclose...\n\n");
    if (llclose(0) == 0) {
        printf("Program ended with llclose.\n\n");
        exit(0);
    }
    
    printf("Failed to end program with llclose.\n\n");
    exit(-1);
}