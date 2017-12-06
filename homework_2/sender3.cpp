#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

#define max 2000

struct packet 
{
    unsigned int no;
    char index[max];
    unsigned int size;
};

int main (int argc, char** argv)
{
    char sbuffer[max];
    char rbuffer[max];
    int sockfd, filefd;
    int n, i;
    unsigned int sended;
    struct hostent* host;
    struct sockaddr_in server;
    socklen_t len;
    struct stat fin;
    fd_set readfd;
    struct timeval t;
    struct packet pac;

    if (argc != 4)
    {
        cout << "Usage: ./sender3 <receiver IP> <receiver port> <file name>" << endl;
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        exit(1);
    }

    host = gethostbyname(argv[1]);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, host->h_addr, host->h_length);
    server.sin_port = htons(atoi(argv[2]));

    filefd = open(argv[3], O_RDONLY);
    if (filefd < 0)
    {
        perror("file open error");
        exit(1);
    }
    fstat(filefd, &fin);

    // send file name
    memset(&sbuffer, 0, sizeof(sbuffer));
    snprintf(sbuffer, sizeof(sbuffer), "%s", argv[3]);
    do
    {
        t.tv_sec = 0;
        t.tv_usec = 100000;
        FD_ZERO(&readfd);
        FD_SET(sockfd, &readfd);

        if (sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            perror("send error");
            exit(1);
        }
    }while(!(select(sockfd+1, &readfd, NULL, NULL, &t)));
    if (recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0)
    {
        perror("receive error");
        exit(1);
    }

    // start to send file
    i = 1;
    sended = 0;
    while(sended < fin.st_size)
    {
        memset(&pac, 0, sizeof(pac));
        memset(rbuffer, 0, sizeof(rbuffer));
        n = read(filefd, pac.index, sizeof(pac.index));
        sended += n;
        pac.no = i++;
        pac.size = n;
        cout << "send: " << sended << endl;

        do
        {
            t.tv_sec = 0;
            t.tv_usec = 100000;
            FD_ZERO(&readfd);
            FD_SET(sockfd, &readfd);

            if (sendto(sockfd, &pac, sizeof(pac), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
            {
                perror("send error");
                exit(1);
            }
        }while(!(select(sockfd+1, &readfd, NULL, NULL, &t)));
        if (recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0)
        {
            perror("receive error");
            exit(1);
        }
    }

    memset(sbuffer, 0, sizeof(sbuffer));
    snprintf(sbuffer, sizeof(sbuffer), "EOF");
    do
    {
        t.tv_sec = 0;
        t.tv_usec = 100000;
        FD_ZERO(&readfd);
        FD_SET(sockfd, &readfd);

        if (sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            perror("send error");
            exit(1);
        }
    }while(!(select(sockfd+1, &readfd, NULL, NULL, &t)));
    if (recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0)
    {
        perror("receive error");
        exit(1);
    }

    cout << "send complete, total packet = " << i-1 << endl;

    close(filefd);
    return 0;
}