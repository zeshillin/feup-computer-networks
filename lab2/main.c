
#include <stdio.h>
#include <stdlib.h>

#include "src/utils.c"
#include "src/connection.c"

// test url: ftp://[eu:pass@]H05T/P4TH

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Input: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    // setup URL data structure
    URL url;
    url.path = malloc(MAX_USER_SIZE);
    url.password = malloc(MAX_PASS_SIZE);
    url.host = malloc(MAX_HOST_SIZE);
    url.path = malloc(MAX_PATH_SIZE);

    if (parseURL(&url, argv[1]) != 0) {
        fprintf(stderr, "Input: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    printf("%s, %s, %s, %s", url.user, url.password, url.host, url.path);

    // take care of host   
    struct hostent *h;
    if ((h = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    char *address = inet_ntoa(*((struct in_addr *) h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", address);

    // estabilish connection
    if (startConnection(address) < 0) {
        printf("Error estabilishing connection.\n");
        return -1;
    }

    return 0;
}