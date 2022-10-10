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
#include "utils.c"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 256

volatile int STOP = FALSE;

struct termios oldtio;
struct termios newtio;
int fd;

//Frame MISC
bool flagged = false;
bool address = false;
bool control = false;
bool protect = false;

void setFrameFalse() {
    bool flagged = false;
    bool address = false;
    bool control = false;
    bool protect = false;
}

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


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    unsigned char buf[BUF_SIZE + 1] = {0};

    if (connectionParameters.role == LlRx) {
        alarmWrapper();
        unsigned char bcc1;
        bool flagged = false;
        bool address = false;
        bool control = false;
        bool protect = false;
        while ((STOP == FALSE) && (alarmCount < 4))  
        {  
            // Returns after 1 chars have been input
            int bytes = read(fd, buf, BUF_SIZE);
            switch (bytes) {            
                case FLAG:
                    if ((flagged) && (address) && (control) && protect) {
                        STOP == true;
                        //send ACK or NACK
                        break;
                    }  
                    else {
                       setFrameFalse();
                       flagged = true; 
                    }
                    break;
                case ADD_TX_AND_BACK:
                    if ((address) || (control) || (protect)) {
                        setFrameFalse();
                    }
                    else if (flagged) {
                        address = true;
                    }
                    else {
                        setFrameFalse();
                    }
                    break;
                case CTRL_SET:
                    if (control || protect) {
                         setFrameFalse();
                        }
                        else if ((flagged) && (address)) {
                            control = true;
                        }
                        else {
                            setFrameFalse();
                        }
                    break;
                case CTRL_DC:
                    if (control || protect) {
                     setFrameFalse();  
                    }
                    else if ((flagged) && (address)) {
                        control = true;
                        STOP == true;
                    }
                    else {
                        setFrameFalse();
                    }
                    break;
                case CTRL_UA:
                    if (control || protect) {
                        setFrameFalse();
                    }
                    else if ((flagged) && (address))
                        control = true;
                    else {
                        setFrameFalse();
                    }
                    break;
                case CTRL_RR():
                    if (control || protect) {
                        setFrameFalse();  
                    }
                    else if ((flagged) && (address)) {
                        control = true;
                        //send positiveACK
                    }
                    else {
                        setFrameFalse();
                    }
                    break;
                case CTRL_REJ():
                    if (control || protect) {
                        setFrameFalse();
                    }
                    else if ((flagged) && (address)) {
                        control = true;
                        //send negativeACK
                    }
                    else {
                        setFrameFalse();
                    }
                    break;
                default:
                    if (flagged && control && address && (!protect)) {
                        bcc1 = bytes;
                        protect = true;
                        break;
                    }
                    else {
                        setFrameFalse();
                    }
                    break;
            }
              
            buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

            printf(":%s:%d\n", buf, bytes);
      }
    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return 1;

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
int llclose(int showStatistics)
{
    return 0;
}
