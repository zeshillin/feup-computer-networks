#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int parseURL(URL *url, char *input) {

    char* URLToken;
    char* auxInput = strdup(input);
    // compare to check if ftp:// is substring
    
    // parse user info 
    if (strstr(input, "@") == NULL) {
        url->user = "anonymous";
        url->password = "anonymous";
    }
    else {
        // get to the start of the name
        for (int i = 0; i < 7; i++)
            auxInput++;

        if ((URLToken = strtok_r(auxInput, ":", &auxInput)) == NULL) 
            return -1;
        url->user = strdup(URLToken); 

        if ((URLToken = strtok_r(auxInput, "@", &auxInput)) == NULL) 
            return -1;
        url->password = strdup(URLToken);
    }

    if ((URLToken = strtok_r(auxInput, "]", &auxInput)) == NULL) 
            return -1;
    printf("url token: %s\n", URLToken);

    // parse host and path -> NOT WORKING (???)
    if ((URLToken = strtok_r(auxInput, "/", &auxInput)) == NULL) {
            printf("url token2: %s\n", URLToken);
            return -1; 
    }
    url->host = strdup(URLToken);


    if ((URLToken = strtok_r(auxInput, "\0", &auxInput)) == NULL) {
            return -1; 
    }
    url->path = strdup(URLToken);

    return 0;
}