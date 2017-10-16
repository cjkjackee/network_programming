#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define max 1000

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
        printf("usage: %s [input file path] [split token]\n",argc[0]);
        exit(1);
    }

    token = (char*)malloc(max);
    cmd = (char*)malloc(max);
    operand = (char*)malloc(max);

    file = fopen(argc[1], "r");
    snprintf(token,max,"%s",argc[2]);

//************************ file input *************************
    printf("%sInput file %s%s\n",breakline,argc[1],breakline);
    while (1)
    {
		fgets(cmd,max,file);
		if (feof(file))
			break;
		cmd = strtok(cmd," ");
		operand = strtok(NULL,"\n");
        if (!strcmp(cmd,"reverse") || !strcmp(cmd,"split"))
            printf("%s %s\n",cmd,operand);
        func(cmd,operand);
    }

    fclose(file);
    printf("%sEnd of input file %s%s\n",breakline,argc[1],breakline);

//************************ user mode *************************
	operand = (char*)malloc(max);
    printf("%sUser input%s\n",starline,starline);
    while (1)
    {
		fgets(cmd,max,stdin);
		if (!strncmp(cmd,"exit",strlen("exit")))
			break;
		cmd = strtok(cmd," ");
		operand = strtok(NULL,"\n");
        func(cmd,operand);
    }


    free(token);
    free(cmd);
    return 0;
}

void func (char* cmd, char* operand)
{
    if (!strcmp(cmd,"reverse"))
    {
        int top = strlen(operand);

        for (int i=--top;i>=0;--i)
            printf("%c",operand[i]);
        printf("\n");
    }
    else if (!strcmp(cmd,"split"))
    {
       	char *p;
        char *str;
        str = (char*)malloc(max);
        p = strstr(operand,token);
        while(p!=NULL)
        {
            strncpy(str,operand,strlen(operand)-strlen(p));
            if(!(strlen(operand)-strlen(p)))
                snprintf(str,max,"%s","");
            printf("%s",str);
            p += strlen(token)*sizeof(char);
            snprintf(operand,max,"%s",p);
            printf(" ");
            p = strstr(operand,token);
        }
        printf("%s\n",operand);
        free(str);
    }
    else
        printf("Usage: reverse||split [string]\n");

    return;
}
