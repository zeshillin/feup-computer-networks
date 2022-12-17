#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

int openSocket(const int port, char *address) {

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[SYS] Socket opening failed. \n");
        return -1;
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("[SYS] Connecting to socket failed. \n");
        return -1;
    }

    return sockfd;
}

int sendCommand(const int socket, char* cmd) {
    int bytes;

    /*send a string to the server*/
    bytes = write(socket, cmd, strlen(cmd));
    if (bytes < 0) {
        perror("write()");
        return -1;
    }

    return 0;
}

int closeSocket(int socket) {

    if (close(socket) < 0) {
        perror("close()");
        return -1;
    }

    return 0;
}