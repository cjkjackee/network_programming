#include <stdio.h>
#include <stdlib.h>

#define max 50

int main (int argv, char** argc)
{
    FILE *file;
    char token[max];
    char fcmd[max];
    char operand[max];

    if (argv < 2)
    {
        printf("usage: ./homework0 [input file path] [split token]\n");
        exit(1);
    }

    file = fopen(argc[1], "r");
    snprintf(token,max,argc[2]);

    while (fscanf(file,"%s",&fcmd)!=EOF)
    {
        if (strcmp(fcmd,"reverse"))
        {
            fscanf(file,"%s");

        }
        
    }

    fclose(file);

    return 0;
}
