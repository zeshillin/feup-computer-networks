#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int parseURL(URL *url, char *input) {

    char* URLToken;
    // compare to check if ftp:// is substring
    
    // parse user info 
    if (strstr(input, "@") == NULL) {
        url->user = "anonymous";
        url->password = "anonymous";
    }
    else {
        // get to the start of the name
        for (int i = 0; i < 7; i++)
            input++;

        if ((URLToken = strtok(input, ":")) == NULL) 
            return -1;
        url->user = strdup(URLToken); 

        if ((URLToken = strtok(NULL, "@")) == NULL) 
            return -1;
        url->password = strdup(URLToken);
    }

    if ((URLToken = strtok(NULL, "]")) == NULL) 
            return -1;
    printf("url token: %s\n", URLToken);

    // parse host and path -> NOT WORKING (???)
    if ((URLToken = strtok(NULL, "/")) == NULL) 
            return -1; 
    url->host = strdup(URLToken);

    if ((URLToken = strtok(NULL, "\0")) == NULL) 
            return -1; 
    
    url->path = strdup(URLToken);


    return 0;
}