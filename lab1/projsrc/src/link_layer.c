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

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    enum state state = START;
    bool acknowledged = false;

    unsigned int ctrl, address;

    unsigned char buf[BUF_SIZE + 1] = {0};

    if (connectionParameters.role == LlRx) {
        //alarmWrapper();
        unsigned char bcc1;

        while ((state != END) && (alarmCount < 4))  
        {   
            // Returns after 1 chars have been input
            int bytes = read(fd, buf, BUF_SIZE);

            switch (state) {            
                case START:
                    if (bytes == FLAG) 
                        state = FLAG_RCV;
                    
                    break;
                case FLAG_RCV:
                    if (bytes == ADD_TX_AND_BACK) {
                        state = ADDRESS;
                        address = bytes;
                    }
                    else if (bytes == FLAG)
                        state = START;
                    else state = START;

                    break;
                case ADDRESS:
                    if (bytes == FLAG) 
                        state = FLAG_RCV;
                    else {
                        if (bytes == CTRL_UA) 
                            acknowledged = true;
                        ctrl = bytes;

                        state = CTRL;
                    }
                    break;
                case CTRL:
                    if ((address ^ ctrl) != 0) {

                    }
                    break;
                default:
                    
                    break;
            }
              
            buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

            printf(":%s:%d\n", buf, bytes);
            alarmCount++;
      }


    return 0;
    }
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
