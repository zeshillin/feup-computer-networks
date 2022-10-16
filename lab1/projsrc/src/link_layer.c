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
int fd;
int cur_seqNum;

//Frame MISC
typedef enum { START, FLAG_RCV, ADDRESS, CTRL, BCC, END } state;

// Alarm MISC
int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

// Set alarm function handler, to be used while receiving data
int alarmWrapper() {
    (void)signal(SIGALRM, alarmHandler);

    while (alarmCount < 4)
    {
        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }
    }
    return 0;
}

u_int8_t readSUFrame(int fd, LinkLayerRole role) {
    state state = START;
    u_int8_t ctrl = 0, address = (role == LlRx) ? ADD_RX_AND_BACK : ADD_TX_AND_BACK;

    u_int8_t buf;
    int bytes;

    while (state != END)  
    {   
        bytes = read(fd, &buf, 1);
        if (bytes == -1) {

            printf("receive ended with -1");
            return -1;
        }
        else if (bytes == 0) { 
            
            printf("receive ended with 0");
            return 0;
        }

        switch (state) {            
            case START:
                printf("start received");
                if (buf == FLAG) 
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
                printf("flag rcv");
                if (buf == address) {
                    state = ADDRESS;

                }
                else if (buf == FLAG)
                    state = FLAG_RCV;
                else state = START;
                break;
            case ADDRESS:
                printf("address received");
                if (buf == FLAG) 
                    state = FLAG_RCV;
                else {
                    state = CTRL;
                    ctrl = buf;
                }
                break;
            case CTRL:
                if (buf == (address ^ ctrl)) 
                    state = BCC;
                else if (buf == FLAG)
                    state = FLAG_RCV;
                else 
                    state = START;

                break;
            case BCC:
                printf("bcc received");
                if (buf == FLAG)
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
    printf("tried to send frame with func");

    frame[0] = FLAG;
    frame[1] = address;
    frame[2] = msg;
    frame[3] = bcc;
    frame[4] = FLAG;

    return write(fd, frame, 5);
}

u_int8_t readIFrame(int fd, unsigned char *buf, int seqNum) {
    state state = START;
    u_int8_t ctrl = SEQNUM_TO_CONTROL(seqNum);

    int bytes;

    return bytes;
    
}
int sendIFrame(int fd, const unsigned char *buf, int length, int seqNum) {

    //setting up frame components
    u_int8_t address = ADD_TX_AND_BACK;
    u_int8_t ctrl = SEQNUM_TO_CONTROL(seqNum);
    u_int8_t bcc1 = ADD_TX_AND_BACK ^ ctrl;
    u_int8_t bcc2 = buf[0];

    //concocting the frame 
    //dynamic array to store frame for resizing purposes (stuffing)
    dArray frame;
    initArray(&frame, 6);
    insertArray(&frame, FLAG);
    insertArray(&frame, ADD_TX_AND_BACK);
    insertArray(&frame, ctrl);
    insertArray(&frame, bcc1);

    for (int i = 1; i < length; i++) 
        bcc2 = (bcc2 ^ buf[i]);
    
    insertArray(&frame, bcc2);

    stuffFrame(&frame);

    insertArray(&frame, FLAG);

    int ret = write(fd, frame.array, frame.size);

    freeArray(&frame);

    return ret;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    printf("started llopen");

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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

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

    //if we're receiving
    if (connectionParameters.role == LlRx) {
        printf("llrx if");
        //wait for frame to arrive
        while (readSUFrame(fd, LlTx) != CTRL_SET) { }
        sendSUFrame(fd, LlTx, CTRL_UA);
    }

    //if we're transmitting
    else {
        int nTries = 0;
        alarmCount = 0;

        while (nTries < connectionParameters.nRetransmissions) {
            printf("sending frame");
            
            while (alarmCount < connectionParameters.timeout) {
                sendSUFrame(fd, LlTx, CTRL_SET);
                printf("sent frame");
                if (readSUFrame(fd, LlTx) == CTRL_UA) {
                    printf("received frame");
                    return 1;
                }
                sleep(3);
                alarmCount++;
             }
             nTries++;
        }
        printf("llopen too many tries");
        return -2;
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
//wrap this funciton around an --while (llwrite == -1) 
int llwrite(LinkLayer connectionParameters, const unsigned char *buf, int bufSize)
{
    int nTries = 0;
    int nRetransmissions = 0;
    alarmCount = 0;
    int seqNum = connectionParameters.serialPort[strlen(connectionParameters.serialPort) - 1];
    
    u_int8_t response; 

    while (nTries < connectionParameters.nRetransmissions) {
        sendIFrame(fd, buf, bufSize, seqNum);
        printf("sending iframe");
        while (alarmCount < connectionParameters.timeout) {
                if ((response = readSUFrame(fd, connectionParameters.role)) == CTRL_RR(seqNum)) {
                    seqNum = seqNum ? 0 : 1;
                    return 1;
                }
                else if ((response) = CTRL_REJ(seqNum))
                    return -1;

                sleep(3);
                alarmCount++;
        }
        nTries++;
    }

    printf("llwrite too many tries");
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) //using as fd
{
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
    return 0;
}
