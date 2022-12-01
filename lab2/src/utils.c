#include "utils.h"

#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int checkFTP(char* input) {

}

int parseURL(URL *url, char *input) {

    char* URLToken;
    char* latterToken;               // these two are here purely for strtok purposes, 
    char* auxInput = strdup(input);  // strange bug was making host and path string unaccessible
    
    // compare to check if ftp:// is substring with checkFTP()
    
    // parse user info 
    if (strstr(input, "@") == NULL) {
        url->user = "anonymous";
        url->password = "anonymous";

        for (int i = 0; i < 6; i++) 
            input++;

        // get host
        if ((latterToken = strtok(input, "/")) == NULL) 
            return -1; 
        url->host = strdup(latterToken);
        printf("latter token: %s\n", latterToken);

        // get path
        if ((latterToken = strtok(NULL, "\0")) == NULL) 
                return -1; 
        url->path = strdup(latterToken);
    }
    else {
        // get to the start of the name
        for (int i = 0; i < 7; i++)
            auxInput++;

        // get name
        if ((URLToken = strtok(auxInput, ":")) == NULL) 
            return -1;
        url->user = strdup(URLToken); 

        // get password
        if ((URLToken = strtok(NULL, "@")) == NULL) 
            return -1;
        url->password = strdup(URLToken);

        // get host
        if ((latterToken = strtok(input, "]")) == NULL) 
            return -1;
        printf("latter token: %s\n", latterToken);
        if ((latterToken = strtok(NULL, "/")) == NULL) 
            return -1; 
        url->host = strdup(latterToken);
        printf("latter token: %s\n", latterToken);

        // get path
        if ((latterToken = strtok(NULL, "\0")) == NULL) 
                return -1; 
        
        url->path = strdup(latterToken);
    }


    return 0;
}