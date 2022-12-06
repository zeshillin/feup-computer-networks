#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "socket.c"

#define SERVER_PORT 21

// reply misc
#define MAX_REPLY_SIZE 1024
#define MULTI_LINE_FLAG '-'
#define LAST_LINE_FLAG ' '

int readReply(int socket, char* code) {

    char reply_size = 0;
    char ch;
    char line_code[3];

    int line_idx = 0;
    bool multi_line = false;
    bool last_line = false;

    if (read(socket, &code, 3) < 0) {
        printf("Error reading reply.\n");
        return -1;
    } 

    /* for (int i = 0; i < 3; i++) {
        if (read(socket, code + i, 1) < 0) {
            printf("Error reading reply.\n");
            return -1;
        } 
    } */
    
    if (read(socket, &ch, 1) < 0) {
        printf("Error reading reply.\n");
        return -1;
    } 
    multi_line = (ch == MULTI_LINE_FLAG);

    if (!multi_line) {

        do {
            if (read(socket, &ch, 1) < 0) {
                printf("Error reading reply.\n");
                return -1;
            } 
            printf("%c", ch);

            reply_size++;
            if (reply_size > MAX_REPLY_SIZE) {
                printf("Reply was too big.\n");
                return -1;
            }

        } while (ch != EOF);

    }
    else {

        // read every line until last line
        do {
            if (read(socket, &ch, 1) < 0) {
                printf("Error reading reply.\n");
                return -1;
            } 
            printf("%c", ch);

            reply_size++;
            if (reply_size > MAX_REPLY_SIZE) {
                printf("Reply was too big.\n");
                return -1;
            }

            if (line_idx < 3) {
                line_code[line_idx] = ch;
            }
            else if (line_idx == 3)
                last_line = (ch == LAST_LINE_FLAG) && (strcmp(line_code, code) == 0);
            
            line_idx++;

            if (ch == '\n') {
                line_idx = 0;
                continue;
            }

        } while (ch != EOF);

    } 
    
    if (!last_line) {
        printf("\nReply structure unknown, exiting...\n");
        return -1;
    }

    return 0;
}

int startConnection(char* address) {

    // estabilish connection with socket
    int socket = openSocket(SERVER_PORT, address);
    if (socket < 0) {
        printf("Socket creation failed.\n");
        return -1;
    }

    // get reply from server
    char reply_code[3];
    if (readReply(socket, reply_code) < 0) {
        return -1;
    };

    return 0;
    
}