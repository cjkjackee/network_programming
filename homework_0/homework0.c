#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define max 50

char *token;

void func(char*, char*);

int main (int argv, char** argc)
{
    FILE *file;
    char breakline[] = "------------------------------";
    char starline[] = "*************************";
    char *cmd;
    char *operand;

//**************** init ************************************
    if (argv < 2)
    {
        printf("usage: ./homework0 [input file path] [split token]\n");
        exit(1);
    }

    token = (char*)malloc(max);
    cmd = (char*)malloc(max);
    operand = (char*)malloc(max);

    file = fopen(argc[1], "r");
    snprintf(token,max,"%s",argc[2]);

//************************ file input *************************
    printf("%sInput file %s%s\n",breakline,argc[1],breakline);
    while (fscanf(file,"%s",cmd)!=EOF)
    {
        fscanf(file,"%s",operand);
        if (!strcmp(cmd,"reverse") || !strcmp(cmd,"split"))
            printf("%s %s\n",cmd,operand);
        func(cmd,operand);
    }

    fclose(file);
    printf("%sEnd of input file %s%s\n",breakline,argc[1],breakline);

//************************ user mode *************************
    printf("%sUser input%s\n",starline,starline);
    while (1)
    {
        scanf("%s",cmd);
        if (!strcmp(cmd,"exit"))
            break;
        scanf("%s",operand);
        func(cmd,operand);
    }


    free(token);
    free(cmd);
    free(operand);
    return 0;
}

void func (char* cmd, char* operand)
{
    if (!strcmp(cmd,"reverse"))
    {
        char *stack;
        int top = 0;

        stack = (char*)malloc(max);
        snprintf(stack,max,"%s0",operand);
        snprintf(operand,max,"%s",stack);
        free(stack);

        for (top=0;operand[top]!='0';++top);
        for (int i=--top;i>=0;--i)
            printf("%c",operand[i]);
        printf("\n");
    }
    else if (!strcmp(cmd,"split"))
    {
        char *p;
        p = strtok(operand,token);
        while(p!=NULL)
        {
            printf("%s ",p);
            p = strtok(NULL,token);
        }
        printf("\n");
    }

    return;
}
