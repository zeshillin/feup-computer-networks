#include "stdio.h"

#define MAX_USER_SIZE 128
#define MAX_PASS_SIZE 128
#define MAX_HOST_SIZE 256
#define MAX_PATH_SIZE 1024
#define MAX_INPUT_SIZE 1536

typedef struct {
    char *user;
    char *password;
    char *host; 
    char *path;
    char *filename;
} URL;

URL url;

/*
    // The struct hostent (host entry) with its terms documented
    struct hostent {
        char *h_name;    // Official name of the host.
        char **h_aliases;    // A NULL-terminated array of alternate names for the host.
        int h_addrtype;    // The type of address being returned; usually AF_INET.
        int h_length;    // The length of the address in bytes.
        char **h_addr_list;    // A zero-terminated array of network addresses for the host.
        // Host addresses are in Network Byte Order.
    };

    #define h_addr h_addr_list[0]	The first address in h_addr_list.

int parseURL(URL *url, char *input);
*/

int checkFTP(char* input);

int parseURL(URL *url, char *input);