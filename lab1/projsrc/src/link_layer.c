// Link layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256
#define Rx = 0
#define Tx = 1

//volatile int STOP = FALSE;

struct termios oldtio;
struct termios newtio;
LinkLayer ll_connectionParameters;

int fd;
int seqNum;

//Frame MISC
typedef enum { START, FLAG_GOT, ADDRESS_GOT, CTRL_GOT, BCC1_GOT, DATA_GOT, BCC2_GOT, END } state;

// Alarm MISC
int alarmEnabled = FALSE;
int alarmCount = 0;
bool timeExceeded = FALSE;

// Alarm function handler
void alarmHandler(int signal)
{   
    timeExceeded = TRUE;
    alarmEnabled = FALSE;
    alarmCount++;

    if (ll_connectionParameters.role == LlTx) {
        if (alarmCount == ll_connectionParameters.nRetransmissions)
            printf("Too many retries: %i\n\n", ll_connectionParameters.nRetransmissions);
        else
            printf("Retry nº: #%d\n", alarmCount);
    }
}

u_int8_t readSUFrame(int fd, LinkLayerRole role) {

    state state = START;
    u_int8_t ctrl = 0, address = (role == LlRx) ? ADD_RX_AND_BACK : ADD_TX_AND_BACK;

    u_int8_t read_buf;
    int bytes;

    timeExceeded = FALSE;

    while (state != END)  
    {   
        if (timeExceeded) {return -1; }
        
        (void)signal(SIGALRM, alarmHandler);
        if (alarmEnabled == FALSE) {
            alarm(ll_connectionParameters.timeout); // Set alarm to be triggered in 4s
            alarmEnabled = TRUE;
        }

        bytes = read(fd, &read_buf, 1);
        if (bytes == -1) {
            return -1;
        }
        /* having this nulls the alarm 
        else if (bytes == 0) { 
            
            printf("receive ended with 0 (no bytes read)");
            return 0;
        }
        */
        switch (state) {            
            case START:
                if (read_buf == FLAG) 
                    state = FLAG_GOT;
                break;
            case FLAG_GOT:
                if (read_buf == address) {
                    state = ADDRESS_GOT;
                }
                else if (read_buf == FLAG)
                    state = FLAG_GOT;
                else state = START;
                break;
            case ADDRESS_GOT:
                if (read_buf == FLAG) 
                    state = FLAG_GOT;
                else {
                    state = CTRL_GOT;
                    ctrl = read_buf;
                }
                break;
            case CTRL_GOT:
                if (read_buf == (address ^ ctrl)) 
                    state = BCC1_GOT;
                else if (read_buf == FLAG)
                    state = FLAG_GOT;
                else 
                    state = START;

                break;
            case BCC1_GOT:
                if (read_buf == FLAG)
                    state = END; 
                else state = START;    
                break;
            default:
                
                break;
        }
            
    }

    return ctrl;
}
int sendSUFrame(int fd, LinkLayerRole role, u_int8_t msg) {

    u_int8_t frame[5];
    u_int8_t address = (role == LlRx) ? ADD_RX_AND_BACK : ADD_TX_AND_BACK;
    u_int8_t bcc = address ^ msg;

    frame[0] = FLAG;
    frame[1] = address;
    frame[2] = msg;
    frame[3] = bcc;
    frame[4] = FLAG;

    return write(fd, frame, 5);
}

//return -1 if read returned an error, -2 if wrong header, -3 if wrong seqNum, -4 if invalid bcc2
u_int8_t readIFrame(int fd, unsigned char *buf, int seqNum) {
    state state = START;
    dArray frame;
    initArray(&frame, 6);

    u_int8_t address = ADD_TX_AND_BACK;
    u_int8_t cur_ctrl = SEQNUM_TO_CONTROL(seqNum);
    int next_seqNum = seqNum ? 0 : 1;
    u_int8_t next_ctrl = SEQNUM_TO_CONTROL(next_seqNum);

    u_int8_t read_buf;
    int bytes;

    timeExceeded = FALSE;

    while (state != END) {
        if (timeExceeded) { return -1; }

        (void)signal(SIGALRM, alarmHandler);
        if (alarmEnabled == FALSE) {
            alarm(ll_connectionParameters.timeout); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }
        
        bytes = read(fd, &read_buf, 1);
        if (bytes == -1) {
            printf("Read function returned an error (-1)\n\n");
            return -1;
        }
        /* having this nulls the alarm
        else if (bytes == 0) { 
            printf("readIFrame ended with 0 (no bytes read)\n");
            freeArray(&frame);
            return 0;
        }
        */
        insertArray(&frame, read_buf);

        switch(state) {
            case START:
                
                if (read_buf == FLAG) 
                    state = FLAG_GOT;
                break;
            case FLAG_GOT:
                if (read_buf == FLAG) {
                    state = END;
                }
                break;

            default:
                break;
        }
    }

    destuffFrame(&frame);
    u_int8_t bcc2 = generateBCC2(&frame);
    state = START;

    int i = 0;

    while (state != END || i != frame.used) {
        switch (state) {
            case START:
                if (frame.array[i] == FLAG)  
                    state = FLAG_GOT;
                else {
                    printf("Error reading Iframe (wrong header)\n\n");
                    return -2;
                }
                break;
            case FLAG_GOT:
                if (frame.array[i] == address)
                    state = ADDRESS_GOT;
                else {
                    printf("Error reading Iframe (wrong header)\n\n");
                    return -3;
                }
                break;
            case ADDRESS_GOT:
                if (frame.array[i] == cur_ctrl) 
                    state = CTRL_GOT;
                
                else if (frame.array[i] == next_ctrl) {
                    printf("Error reading Iframe (wrong sequence number)\n\n");
                    return -2;
                }
                else {
                    printf("Error reading Iframe (wrong header)\n\n");
                    return -3;
                }
                break;
            case CTRL_GOT:
                if (frame.array[i] == (address^cur_ctrl)) 
                    state = BCC1_GOT;
                else if (frame.array[i]== (address^next_ctrl)) {
                    printf("Error reading Iframe (wrong sequence number)\n\n");
                    return -3;
                }
                else {
                    printf("Error reading Iframe (wrong header)\n\n");
                    return -2;
                }
                break;

            //data 
            case BCC1_GOT:
                if (i == frame.used - 2) {
                    if (frame.array[i] == bcc2) 
                        state = BCC2_GOT;
                    else {
                        printf("Error reading Iframe (invalid BCC2)\n\n");
                        return -4;
                    }
                }
            
                break;

            case BCC2_GOT:
                if (frame.array[i] == FLAG)
                    state = END;
                else {
                    printf("Error reading Iframe: wrong header\n\n");
                    return -2;
                }
                break;

            default:
                break;
        }
        i++;
    }

    dArray data = getData(&frame);
    freeArray(&frame);

    memcpy(buf, data.array, data.used);

    return data.used;
    
}
int sendIFrame(int fd, const unsigned char *buf, int length, int seqNum) {

    //setting up frame components
    u_int8_t ctrl = SEQNUM_TO_CONTROL(seqNum);
    u_int8_t bcc1 = ADD_TX_AND_BACK ^ ctrl;
    u_int8_t bcc2 = 0;

    //concocting the frame 
    //dynamic array to store frame for resizing purposes (stuffing)
    dArray frame;
    initArray(&frame, 6);

    insertArray(&frame, FLAG);
    insertArray(&frame, ADD_TX_AND_BACK);
    insertArray(&frame, ctrl);
    insertArray(&frame, bcc1);

    for (int i = 0; i < length; i++) {
        //printf("insert %x", buf[i]);
        insertArray(&frame, buf[i]);
        bcc2 ^= buf[i];
    }
    insertArray(&frame, bcc2);

    stuffFrame(&frame);
    insertArray(&frame, FLAG);

    int ret = write(fd, frame.array, frame.used);

    freeArray(&frame);

    return ret;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    ll_connectionParameters = connectionParameters;

    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = connectionParameters.serialPort;
    // Open serial port device for writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        return(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = connectionParameters.timeout; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 1 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    seqNum = 0;

    alarmCount = 0;

    //if we're receiving
    if (connectionParameters.role == LlRx) {
        //wait for frame to arrive
        while (readSUFrame(fd, LlTx) != CTRL_SET) { }
        sendSUFrame(fd, LlTx, CTRL_UA);
    }
    //if we're transmitting
    else if (connectionParameters.role == LlTx) {

        while (alarmCount < connectionParameters.nRetransmissions) {
            sendSUFrame(fd, LlTx, CTRL_SET);
            
            if (readSUFrame(fd, LlTx) == CTRL_UA) {
                printf("Received acknowledgement frame. Continuing...\n\n");
                return 1;
            }
        }
        return -2;
    }
    else {
        printf("No role could be discerned. Exiting...\n");
        exit(1);
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarmCount = 0;
    int old_seqNum = seqNum; //connectionParameters.sequenceNumber;
    int next_seqNum = seqNum ? 0 : 1;
    
    int bytes;
    u_int8_t response; 

    while (alarmCount < ll_connectionParameters.nRetransmissions) {
        
        printf("Sending Iframe...\n");
        if ((bytes = sendIFrame(fd, buf, bufSize, old_seqNum)) < 0) {
            printf("Problem sending IFrame (0 bytes sent).\n\n");
            return -2;
        }

        while (response != CTRL_RR(next_seqNum)) {
            if ((response = readSUFrame(fd, ll_connectionParameters.role)) == CTRL_RR(next_seqNum)) {
                printf("Iframe acknowledged correctly. Continuing...\n\n");
                seqNum = next_seqNum;
                return bytes;
            }
            if ((response) == CTRL_REJ(old_seqNum)) {   
                printf("Iframe rejected.\n\n"); 
                alarmCount = 0;
                break;
            }  
            if ((bytes = sendIFrame(fd, buf, bufSize, old_seqNum)) < 0) {
                printf("Problem sending IFrame (0 bytes sent).\n\n");
                return -2;
            }
        }
    }

    printf("LLWrite failure (too many tries).\n\n");
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    //if read successful, change seqNum
    int bytes;
    
    printf("Reading Iframe...\n");
    while ((bytes = (readIFrame(fd, packet, seqNum))) < 0) {
        switch (bytes) {
            case -1:
                return -1;
            case -2:
                printf("Sending rejection SU frame...\n\n");
                sendSUFrame(fd, LlTx, CTRL_REJ(seqNum));
                break;
            case -4:
                printf("Sending rejection SU frame...\n\n");
                sendSUFrame(fd, LlTx, CTRL_REJ(seqNum));
                break;
            default: 
                break;
        }
    }

    //for testing purposes -> sendSUFrame(fd, LlTx, CTRL_REJ(seqNum));
    seqNum = seqNum ? 0 : 1;
    printf("Sending IFrame reception acknowledgement SU frame...\n\n");
    sendSUFrame(fd, LlTx, CTRL_RR(seqNum));
    return bytes;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) //using as fd
{
    printf("Using LLClose... \n");
    
    u_int8_t msg;

    alarmCount = 0;
    
    if (ll_connectionParameters.role == LlRx) {
        while (alarmCount < ll_connectionParameters.nRetransmissions) {
            if ((msg = readSUFrame(fd, LlTx)) == CTRL_DC) {
                sendSUFrame(fd, LlTx, CTRL_DC);
                break;
            }      
        }
    }
    else {
        sendSUFrame(fd, LlTx, CTRL_DC);
        while (alarmCount < ll_connectionParameters.nRetransmissions) {
            if (alarmEnabled == FALSE) {
                alarm(ll_connectionParameters.timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }
            if ((msg = readSUFrame(fd, LlTx)) == CTRL_DC) {
                sendSUFrame(fd, LlTx, CTRL_UA);
                break;
            }
        
        }
    }

    if (alarmCount == ll_connectionParameters.nRetransmissions) {
        printf("LLclose failed (too many tries).\n");
        return -1;
    }

    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);

    return 0;
}
