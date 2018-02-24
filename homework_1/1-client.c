#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define MAXDATA 1500

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "useage %s hostname port\n", argv[0]); //user defined error
        exit(0);
    }

    int sockfd = 0, serv_port;
    struct sockaddr_in servaddr;
    struct hostent *server;
    int maxfd;
    fd_set rset, allset;


    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("ERROR in opening socket"); //because of socket() can change the errno
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    serv_port = atoi(argv[2]);
    servaddr.sin_port = htons(serv_port);
    server = gethostbyname(argv[1]);
    servaddr.sin_addr.s_addr = ((struct in_addr*)server->h_addr)->s_addr;
    printf("%s\n", inet_ntoa(servaddr.sin_addr));

    int err = 0;
    err = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(err == -1)
    {
        perror("ERROR in connect()");
        exit(1);
    }
    printf("connect() success\n");
    maxfd = sockfd;
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(fileno(stdin), &allset);
    FD_SET(sockfd, &allset);
    printf("sockfd = %d\n", sockfd);

    char send_data[MAXDATA], recv_data[MAXDATA];
    bzero(&recv_data, MAXDATA);
    bzero(&send_data, MAXDATA);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    for(;;)
    {
        rset = allset;
        //maxfd = fileno(stdin) > sockfd ? fileno(stdin)+1 : sockfd+1;
        if((select(maxfd+1, &rset, NULL, NULL, NULL)) == -1)
        {
            perror("select() failed");
            exit(1);
        }

        for(int i = 0; i <= maxfd; i++)
        {
            if(FD_ISSET(i, &rset))
            {
                if(i == sockfd)
                {
                    //printf("some msg is coming from the server...\n");
					int n;
                    bzero(&recv_data, MAXDATA);
                    n = recv(sockfd, recv_data, sizeof(recv_data), 0);
                    if(n <= 0)
                    {
                        fprintf(stderr, "ERROR: server terminated");
                        close(sockfd);
                        break;
                    }
                    printf("%s", recv_data);
                }

                else if(i == fileno(stdin))
                {
                    bzero(&send_data, MAXDATA);
                    fscanf(stdin, "\n%[^\n]", send_data); //read until the last white space(new line)
                    if(strcmp(send_data, "exit") == 0)
                    {
                        close(sockfd);
                        break;
                    }
                    if((send(sockfd, send_data, sizeof(send_data), 0)) < 0)
                        fprintf(stderr, "ERROR: send() failed");
                }
            }

        }
    }
    close(sockfd);

    return 0;
}
