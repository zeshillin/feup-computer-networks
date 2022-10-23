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
int cur_seqNum;

//Frame MISC
typedef enum { START, FLAG_GOT, ADDRESS_GOT, CTRL_GOT, BCC1_GOT, DATA_GOT, BCC2_GOT, END } state;

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

    u_int8_t read_buf;
    int bytes;

    while (state != END)  
    {   
        bytes = read(fd, &read_buf, 1);
        if (bytes == -1) {

            printf("receive ended with -1");
            return -1;
        }
        else if (bytes == 0) { 
            
            printf("receive ended with 0 (no bytes read)");
            return 0;
        }

        switch (state) {            
            case START:
                printf("start received");
                if (read_buf == FLAG) 
                    state = FLAG_GOT;
                break;
            case FLAG_GOT:
                printf("flag rcv");
                if (read_buf == address) {
                    state = ADDRESS_GOT;
                }
                else if (read_buf == FLAG)
                    state = FLAG_GOT;
                else state = START;
                break;
            case ADDRESS_GOT:
                printf("address received");
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
                printf("bcc received");
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
    printf("tried to send frame with func");

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
    int next_seqNum = cur_seqNum ? 0 : 1;
    u_int8_t next_ctrl = SEQNUM_TO_CONTROL(next_seqNum);

    u_int8_t read_buf;
    int bytes;

    printf("start readiframe\n");
    while (state != END) {
        printf("one octet insertion\n");
        bytes = read(fd, &read_buf, 1);
        if (bytes == -1) {
            printf("readIFrame ended with error");
            return -1;
        }
        else if (bytes == 0) { 
            printf("readIFrame ended with 0 (no bytes read)");
            freeArray(&frame);
            return 0;
        }
        printf("%d\n", read_buf);
        insertArray(&frame, read_buf);

        switch(state) {
            case START:
                if (read_buf == FLAG) 
                    state = FLAG_GOT;
                break;
            case FLAG_GOT :
                if (read_buf == FLAG) 
                    state = END;
                break;

            default: 
                break;
        }
    }

    printf("destuff frame \n");
    destuffFrame(&frame);
    printf("after destuff\n");
    u_int8_t bcc2 = generateBCC2(&frame);
    printf("after bcc2\n");
    state = START;

    int i = 0;

    while (state != END || i < frame.used) {
       
        switch (state) {
            case START:
                if (frame.array[i] == FLAG)  
                    state = FLAG_GOT;
                else {
                    printf("error reading Iframe (wrong header)");
                    return -2;
                }
                break;
            case FLAG_GOT:
                if (frame.array[i] == address)
                    state = ADDRESS_GOT;
                else {
                    printf("error reading Iframe (wrong header)");
                    return -3;
                }
                break;
            case ADDRESS_GOT:
                if (frame.array[i] == cur_ctrl) 
                    state = CTRL_GOT;
                
                else if (frame.array[i] == next_ctrl) 
                    return -2;
                else {
                    printf("error reading Iframe (wrong header)");
                    return -3;
                }
                break;
            case CTRL_GOT:
                if (frame.array[i] == (address^cur_ctrl)) 
                    state = BCC1_GOT;
                else if (frame.array[i]== (address^next_ctrl)) {
                    printf("error reading Iframe (wrong sequence number)");
                    return -3;
                }
                else {
                    printf("error reading Iframe (wrong header)");
                    return -2;
                }
                break;

            //data 
            case BCC1_GOT:
                if (i == frame.used - 2) {
                    if (frame.array[i] == bcc2) 
                        state = BCC2_GOT;
                    else {
                        printf("error reading Iframe (invalid BCC2)");
                        return -4;
                    }
                }
                else {
                   continue;
                }
                break;

            case BCC2_GOT:
                if (frame.array[i] == FLAG)  
                    state = END;
                else {
                    printf("error reading Iframe (wrong header)");
                    return -4;
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

    return bytes;
    
}
int sendIFrame(int fd, const unsigned char *buf, int length, int seqNum) {

    //setting up frame components
    u_int8_t ctrl = SEQNUM_TO_CONTROL(seqNum);
    u_int8_t bcc1 = ADD_TX_AND_BACK ^ ctrl;
    u_int8_t bcc2 = buf[0];
    printf("ctrl %x\n", ctrl);
    printf("bcc1 %x\n", bcc1);
    printf("bcc2 %c\n", bcc2);

    //concocting the frame 
    //dynamic array to store frame for resizing purposes (stuffing)
    dArray frame;
    initArray(&frame, 6);

    insertArray(&frame, FLAG);
    insertArray(&frame, ADD_TX_AND_BACK);
    insertArray(&frame, ctrl);
    insertArray(&frame, bcc1);

    for (int i = 1; i < length; i++) {
        //printf("insert %x\n", buf[i]);
        insertArray(&frame, buf[i]);
        bcc2 = (bcc2 ^ buf[i]);
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
    printf("started llopen");

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

    cur_seqNum = 0;

    //if we're receiving
    if (connectionParameters.role == LlRx) {
        printf("llrx if");
        //wait for frame to arrive
        while (readSUFrame(fd, LlTx) != CTRL_SET) { }
        sendSUFrame(fd, LlTx, CTRL_UA);
    }
    //if we're transmitting
    else if (connectionParameters.role == LlTx) {
        printf("Lltx if");
        int nTries = 0;
        alarmCount = 0;

        while (nTries < connectionParameters.nRetransmissions) {
            printf("sending frame");
            sendSUFrame(fd, LlTx, CTRL_SET);
            
            while (alarmCount < connectionParameters.timeout) {
                printf("sent frame");
                if (readSUFrame(fd, LlTx) == CTRL_UA) {
                    printf("received ack frame");
                    return 1;
                }
                sleep(3);
                alarmCount++;
             }
             nTries++;
        }
        return -2;
    }
    else {
        printf("No role could be discerned. Exiting...");
        exit(1);
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int nTries = 0;
    alarmCount = 0;
    int old_seqNum = cur_seqNum; //connectionParameters.sequenceNumber;
    int next_seqNum = cur_seqNum ? 0 : 1;
    
    int bytes;
    u_int8_t response; 

    while (nTries < ll_connectionParameters.nRetransmissions) {
        printf("sending iframe\n");
        if (!(bytes = sendIFrame(fd, buf, bufSize, old_seqNum))) {
            printf("Problem sending IFrame (0 bytes sent).\n");
            return -2;
        }

        while (alarmCount < ll_connectionParameters.timeout) {
                printf("here 1\n");
                if ((response = readSUFrame(fd, ll_connectionParameters.role)) == CTRL_RR(next_seqNum)) {
                    return bytes;
                }
                else if ((response) == CTRL_REJ(old_seqNum)) {
                    if (!sendIFrame(fd, buf, bufSize, old_seqNum)) {
                        printf("Problem sending IFrame (0 bytes sent) (2).\n");
                        return -2;
                    }
                    nTries = 0;
                    break;
                }
                else if ((response == CTRL_RR(old_seqNum)) || (response == CTRL_REJ(old_seqNum))) {
                    continue;
                }
                
                sleep(3);
                alarmCount++;
        }
        nTries++;
    }

    printf("LLWrite failure (too many tries).\n");
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    //if read successful, change seqNum
    int msg;
    printf("before readIframe\n");
    while ((msg = (readIFrame(fd, packet, cur_seqNum))) != 1) {
        printf("after readiframe\n");
        switch (msg) {
            case -1:
                return -1;
            case -2:
                sendSUFrame(fd, LlTx, CTRL_REJ(cur_seqNum));
                break;
            case -3: 
                sendSUFrame(fd, LlTx, CTRL_RR(cur_seqNum));
                break;

            default: 
                break;
        }
    }

    cur_seqNum = cur_seqNum ? 0 : 1;
    sendSUFrame(fd, LlTx, CTRL_RR(cur_seqNum));

    return msg;

}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) //using as fd
{
    int nTries = 0;

    u_int8_t msg;

    if (ll_connectionParameters.role == LlRx) {
        while (nTries < ll_connectionParameters.timeout) {
            if ((msg = readSUFrame(fd, LlTx)) == CTRL_DC) {
                sendSUFrame(fd, LlTx, CTRL_DC);
                break;
            }
            sleep(3);
            alarmCount++;        
        }
    }
    else {
        sendSUFrame(showStatistics, LlTx, CTRL_DC);

        while (nTries < ll_connectionParameters.timeout) {
            if ((msg = readSUFrame(fd, LlTx)) == CTRL_DC) {
                sendSUFrame(fd, LlTx, CTRL_UA);
                break;
            }
            sleep(3);
            alarmCount++;
        }
    }

    if (nTries == ll_connectionParameters.timeout) {
        printf("LLclose failed (too many tries).\n");
        return -1;
    }

    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);

    return 1;
}
