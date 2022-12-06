
#include <stdio.h>
#include <stdlib.h>

#include "src/utils.c"
#include "src/connection.c"

extern URL url;

// test url: ftp://[eu:pass@]H05T/P4TH/name.txt
// timestamp.txt -> ftp://ftp.up.pt/pub/kodi/timestamp.txt

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Input: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    // setup URL data structure
    url.path = malloc(MAX_USER_SIZE);
    url.password = malloc(MAX_PASS_SIZE);
    url.host = malloc(MAX_HOST_SIZE);
    url.path = malloc(MAX_PATH_SIZE);

    if (parseURL(&url, argv[1]) != 0) {
        fprintf(stderr, "Input: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    printf(" User: %s\n Password:%s\n Host:%s\n Path:%s\n Filename:%s\n\n\n", url.user, url.password, url.host, url.path, url.filename);

    // take care of host   
    struct hostent *h;
    if ((h = gethostbyname(url.host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    char *address = inet_ntoa(*((struct in_addr *) h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", address);

    int socket = startConnection(address);
    // estabilish connection
    if (socket < 0) {
        printf("Error estabilishing connection.\n");
        return -1;
    }

    // enter passive mode and open new socket
    int downloadSocket = setupDownload(socket);
    if (downloadSocket < 0) {
        printf("Error sending command.\n");
        return -1;
    }

    return 0;
}