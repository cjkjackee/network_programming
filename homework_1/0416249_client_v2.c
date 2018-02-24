#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define max 1000

int main (int argc, char** argv)
{
    int sockfd;
    fd_set sockset;
    int chk;
    int port;
    char buffer[max];
    struct sockaddr_in server;

    if (argc < 3)
    {
        printf("usage: ./client <SERVER_IP> <SERVER_PORT>\n");
        exit(1);
    }

    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&server.sin_addr);
    port = atoi(argv[2]);
    server.sin_port = htons(port); 
    
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd<0)  
    {
        perror("socket error");
        exit(1);
    }
    
    if ((connect(sockfd,(struct sockaddr*)&server,sizeof(server)))<0)
    {
        perror("connect error");
        exit(1);
    }

    while(1)
    {
        FD_ZERO(&sockset);
        FD_SET(fileno(stdin),&sockset);
        FD_SET(sockfd,&sockset);
        select(sockfd+1,&sockset,NULL,NULL,NULL);
        if (FD_ISSET(fileno(stdin),&sockset))
        {
            fgets(buffer,sizeof(buffer),stdin);
            send(sockfd,buffer,sizeof(buffer),0);
            if (!strncmp(buffer,"exit",4))
                break;
            memset(buffer,0,sizeof(buffer));
        }
        
        if (FD_ISSET(sockfd,&sockset))
        {
            if((chk=recv(sockfd,buffer,sizeof(buffer),0)<0))
                perror("recive error");
            printf("%s",buffer);
        }
    }

    close(sockfd);
    return 0;
}
