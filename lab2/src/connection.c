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
#define MAX_REPLY_SIZE 2048
#define MULTI_LINE_FLAG '-'
#define LAST_LINE_FLAG ' '

URL url;

int readReply(int socket, char* code) {

    char reply_size = 0;
    char ch;
    char reply_code[4] = {'\0'};
    char line_code[4] = {'\0'};

    int line_idx = 0;
    bool multi_line = false;
    bool last_line = false;

    if (read(socket, &reply_code, 3) < 0) {
        printf("Error reading reply.\n");
        return -1;
    } 
    
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

        } while (ch != '\n');
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

            if (ch == '\n') {
                if (last_line) {
                    break;
                }
                line_idx = 0;
                continue;
            }

            if (line_idx < 3) {
                line_code[line_idx] = ch;
            }
            else if (line_idx == 3) {
                last_line = (ch == LAST_LINE_FLAG) && (strcmp(line_code, reply_code) == 0);
            } 
            
            line_idx++;

        } while (ch != EOF);

    
        if (!last_line) {
            printf("\nReply structure unknown, exiting...\n");
            return -1;
        }
    } 

    code = strdup(reply_code);

    return 0;
}

int startConnection(char* address) {

    // estabilish connection with socket
    int socket = openSocket(SERVER_PORT, address);
    if (socket < 0) {
        printf("Socket creation failed.\n");
        return -1;
    }

    printf("\n Connection estabilished!\n\n");

    // get reply from server
    char reply_code[3];
    if (readReply(socket, reply_code) < 0) {
        return -1;
    };

    return socket;
}

int setupDownload(const int socket) {

    // send user
    char* user = malloc(strlen(url.user) + 6);
    strcat(user, "user ");
    strcat(user, url.user);
    strcat(user, "\n");
    if (sendCommand(socket, user) < 0) {
        printf("User sending failed.\n");
        return -1;
    }

    char reply_code[3];
    if (readReply(socket, reply_code) < 0) 
        return -1;

    // send pass
    char* pass = malloc(strlen(url.password) + 6);
    strcat(pass, "pass ");
    strcat(pass, url.password);
    strcat(pass, "\n");
    if (sendCommand(socket, pass) < 0) {
        printf("Password sending failed.\n");
        return -1;
    }

    if (readReply(socket, reply_code) < 0) 
        return -1;
    
    // set pasv
    char* pasv = "pasv\n";
    if (sendCommand(socket, pasv) < 0) {
        printf("Passive mode command sending failed.\n");
        return -1;
    }

    if (readReply(socket, reply_code) < 0) 
        return -1;
    
    


    return 0;

}