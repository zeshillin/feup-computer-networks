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
#define MAX_IP_REPLY__SIZE 48
#define MULTI_LINE_FLAG '-'
#define LAST_LINE_FLAG ' '

// download misc
#define MAX_PACKET_SIZE 512

URL url;

int readReply(int socket, char* code) {

    char reply_size = 0;
    char ch;
    char reply_code[4] = {'\0'};
    char line_code[4] = {'\0'};

    int line_idx = 0;
    bool multi_line = false;
    bool last_line = false;

    // printf("[REPLY]\n\n");
    for (int i = 0; i < 3; i++) {
        if (read(socket, &reply_code[i], 1) < 0) {
            printf("[SYS] Error reading reply.\n");
            return -1;
        } 
        code[i] = reply_code[i];
        printf("%c", reply_code[i]);
    }
    
    if (read(socket, &ch, 1) < 0) {
        printf("[SYS] Error reading reply.\n");
        return -1;
    } 
    printf("%c", ch);

    multi_line = (ch == MULTI_LINE_FLAG);

    if (!multi_line) {

        do {
            if (read(socket, &ch, 1) < 0) {
                printf("[SYS] Error reading reply.\n");
                return -1;
            } 
            printf("%c", ch);

            reply_size++;
            if (reply_size > MAX_REPLY_SIZE) {
                printf("[SYS] Reply was too big.\n");
                return -1;
            }

        } while (ch != '\n');
    }
    else {

        // read every line until last line
        do {
            if (read(socket, &ch, 1) < 0) {
                printf("[SYS] Error reading reply.\n");
                return -1;
            } 
            printf("%c", ch);

            reply_size++;
            if (reply_size > MAX_REPLY_SIZE) {
                printf("[SYS] Reply was too big.\n");
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
            printf("\n[SYS] Reply structure unknown, exiting...\n");
            return -1;
        }
    } 
    /* printf("\n[REPLY]\n"); */

    return 0;
}

int readPortReply(int socket) {

    char reply_text[MAX_IP_REPLY__SIZE] = { '\0' };
    int i = 0;

    char ch;
    char* numToken;

   /*  printf("[REPLY]\n\n"); */
    do {
        if (read(socket, &ch, 1) < 0) {
            printf("[SYS] Error reading reply.\n");
            return -1;
        } 
        printf("%c", ch);

        reply_text[i] = ch;
        i++;            

    } while (ch != '\n');
    /* printf("\n[REPLY]\n"); */

    // skip IP address
    if ((numToken = strtok(reply_text, "(")) == NULL) {
        printf("[SYS] Reply structure unknown.\n");
        return -1;
    }
    for (int i = 0; i < 4; i++) {
        if ((numToken = strtok(NULL, ",")) == NULL) {
            printf("[SYS] Reply structure unknown.\n");
            return -1;
        }
    }
    // calculate new port 
    if ((numToken = strtok(NULL, ",")) == NULL) {
        printf("[SYS] Reply structure unknown.\n");
        return -1;
    }
    int port = atoi(numToken)*256;
    printf("\n[SYS] P1: %s\n", numToken);

    if ((numToken = strtok(NULL, ")")) == NULL) {
        printf("[SYS] Reply structure unknown.\n");
        return -1;
    }
    port += atoi(numToken);
    printf("[SYS] P2: %s\n", numToken);

    printf("[SYS] New port: P1*256 + P2 = %i\n\n", port);

    return port;
}

int readSizeReply(int socket) {
    
    char reply_code[4] = { '\0'};
    char reply_text[MAX_REPLY_SIZE];
    int i = 0;

    char ch;    
    /* printf("[REPLY]\n\n"); */
    for (int i = 0; i < 3; i++) {
        if (read(socket, &reply_code[i], 1) < 0) {
            printf("[SYS] Error reading reply.\n");
            return -1;
        } 
        printf("%c", reply_code[i]);
    }

    do {
        if (read(socket, &ch, 1) < 0) {
            printf("[SYS] Error reading reply.\n");
            return -1;
        } 
        printf("%c", ch);

        reply_text[i] = ch;
        i++;

    } while (ch != '\n');
    /* printf("\n[REPLY]\n"); */

    if (strcmp(reply_code, "150") != 0) {
        printf("[SYS] Couldn't open requested file.\n");
        return -1;
    }

    char* sizeToken; 
    if ((sizeToken = strtok(reply_text, "(")) == NULL) {
        printf("[SYS] Reply structure unknown.\n");
        return -1;
    }
    if ((sizeToken = strtok(NULL, " ")) == NULL) {
        printf("[SYS] Reply structure unknown.\n");
        return -1;
    }

    return atoi(sizeToken);

}

int startConnection(char* address) {

    // estabilish connection with socket
    int socket = openSocket(SERVER_PORT, address);
    if (socket < 0) {
        printf("[SYS] Socket creation failed.\n");
        return -1;
    }

    printf("\n[SYS] Connection estabilished!\n\n");

    // get reply from server
    char reply_code[4] = {'\0'};
    if (readReply(socket, reply_code) < 0) {
        return -1;
    };

    return socket;
}

// returns download socket
int setupDownload(const int socket, char* address) {
    
    // send user
    char* user = malloc(strlen(url.user) + 6);
    strcat(user, "user ");
    strcat(user, url.user);
    strcat(user, "\n");
    printf("\n[COMMAND] %s\n", user);
    if (sendCommand(socket, user) < 0) {
        printf("[SYS] User sending failed.\n");
        return -1;
    }

    char reply_code[4] = {'\0'};
    if (readReply(socket, reply_code) < 0) 
        return -1;

    // send pass
    char* pass = malloc(strlen(url.password) + 6);
    strcat(pass, "pass ");
    strcat(pass, url.password);
    strcat(pass, "\n");
    printf("\n[COMMAND] %s\n", pass);
    if (sendCommand(socket, pass) < 0) {
        printf("[SYS] Password sending failed.\n");
        return -1;
    }

    if (strcmp(reply_code, "230") != 0) {
        printf("[SYS] Login unsuccessful.\n");
        return -1;
    }

    if (readReply(socket, reply_code) < 0) 
        return -1;
    
    // set pasv
    char* pasv = malloc(5);
    strcat(pasv, "pasv\n");
     printf("\n[COMMAND] %s\n", pasv);
    if (sendCommand(socket, pasv) < 0) {
        printf("[SYS] Passive mode command sending failed.\n");
        return -1;
    }

    int port = readPortReply(socket);
    if (port < 0) 
        return -1;
    
    int downloadSocket = openSocket(port, address);
    if (downloadSocket < 0) {
        printf("[SYS] Error opening new download socket.\n");
        return -1;
    }

    return downloadSocket;
}

int download(int socket, int downloadSocket, char* path, char* filename) {

    char* retr = malloc(6 + strlen(path));
    strcat(retr, "retr ");
    strcat(retr, path);
    strcat(retr, "\n");
     printf("[COMMAND] %s\n", retr);
    if (sendCommand(socket, retr) < 0) {
        printf("[SYS] Retr command sending failed.\n");
        return -1;
    }

    int filesize = readSizeReply(socket);
    if (filesize < 0) 
        return -1;

    FILE *f = fopen(filename, "w");
    int read_bytes = 0;

    uint8_t buf[MAX_PACKET_SIZE];
    int bytes;

    do {
        bytes = read(downloadSocket, &buf, MAX_PACKET_SIZE);
        if (bytes < 0) {
            printf("[SYS] Error downloading file.\n");
            return -1;
        }

        fwrite(buf, bytes, 1, f);
        read_bytes += bytes;

        printf("\n[SYS] Download Progress: %i/%i\n", read_bytes, filesize);
    } while (read_bytes < filesize);

    printf("\n");
    if (fclose(f) < 0)
        return -1;

    return 0;
}

int closeConnection(int socket, int downloadSocket) {

    char reply_code[4] = {'\0'};
    if (readReply(socket, reply_code) < 0)
        return -1;

    if (closeSocket(socket) < 0) {
        printf("[SYS] Couldn't end socket connection correctly.\n\n");
        return -1;
    } 

    if (closeSocket(downloadSocket) < 0) {
        printf("[SYS] Couldn't end socket connection correctly.\n\n");
        return -1;
    }

    if (strcmp(reply_code, "226") != 0) {
        printf("[SYS] Transfer completion message not acknowledged.\n\n");
        return -1;
    } 

    free(url.filename);
    free(url.user);
    free(url.password);
    free(url.host);
    free(url.path);
    
    return 0; 
}   