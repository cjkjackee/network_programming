#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    int port;
    int chk;
    char recv_buff[max];
    char send_buff[max];
    struct sockaddr_in server;
    int status;
    pid_t pid;

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
    // TODO: replace the function by using select()
    pid = fork();
    if (!pid)
    {
        while((chk = recv(sockfd,recv_buff,sizeof(recv_buff),0)))
        {
            if(chk<0)
            {
                perror("recive error");
                exit(1);
            }
            if (!chk)
                break;

            printf("%s",recv_buff);
        }
    }
    else 
    {
        while(fgets(send_buff,sizeof(send_buff),stdin))
        {
		    if (send(sockfd,send_buff,strlen(send_buff),0)<0)
                perror("send error");
            if (!strncmp(send_buff,"exit",4))
                break;
            memset(send_buff,0,max);
        }
        wait(&status);
    }
    

    close(sockfd);
    return 0;
}
