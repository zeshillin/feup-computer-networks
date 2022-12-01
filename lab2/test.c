#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{   
    
    char* tok;
    tok = strtok(argv[1], "]");

    tok = strtok(NULL, "/");

    printf("%s\n", tok);

    return 0;
}