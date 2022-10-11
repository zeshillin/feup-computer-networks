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

//Frame MISC
enum state { START, FLAG_RCV, ADDRESS, CTRL, BCC, END };

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

int receiveFrame(int fd, LinkLayerRole role) {
    enum state state = START;

    unsigned int ctrl, address = (role == LlRx) ? ADD_RX_AND_BACK : ADD_TX_AND_BACK;

    unsigned char buf;
    int bytes;

    while (state != END)  
    {   
        if ((bytes = read(fd, &buf, 1)) != 0)
            return -1;

        switch (state) {            
            case START:
                if (buf == FLAG) 
                    state = FLAG_RCV;
                
                break;
            case FLAG_RCV:
                if (buf == address) {
                    state = ADDRESS;
                    address = bytes;
                }
                else if (buf == FLAG)
                    state = START;
                else state = START;

                break;
            case ADDRESS:
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
                else if (buf == address) {
                    state = ADDRESS;
                }

                break;
            case BCC:
                if (bytes == FLAG)
                    state = END;        
                break;
            default:
                
                break;
        }
            
    }

    return ctrl;
}


int sendFrame(int fd, LinkLayerRole role, int msg) {
    unsigned int frame[5];

    frame[0] = FLAG;
    frame[1] = (role == LlRx) ? ADD_RX_AND_BACK : ADD_TX_AND_BACK;
    frame[2] = msg;
    frame[3] = frame[1] ^ frame[2];
    frame[4] = FLAG;

    return write(fd, frame, 5);
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
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
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
        while (receiveFrame(fd, connectionParameters.role) != CTRL_SET) {
            printf("trying");
            continue;
        }
        sendFrame(fd, connectionParameters.role, CTRL_UA);
    }

    //if we're transmitting
    else {
        int nTries = 0;
        
        //try to send frame 3 times before timing out (4 seconds)
        //if frame is received, leave function with 0 status
        alarmWrapper();
        while (nTries < connectionParameters.nRetransmissions) {
            sendFrame(fd, connectionParameters.role, CTRL_SET);

            if (receiveFrame(fd, connectionParameters.role) == CTRL_UA) {
                printf("connection estabilished with llopen()");
                return 1;
            }
            else nTries++;
        }

        return -1; 
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

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
    close(fd);
    return 0;
}
