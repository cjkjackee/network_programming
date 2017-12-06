#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

#define max 2000
bool resend = true;

struct packet
{
    unsigned int no;
    char index[max];
    unsigned int size;
};

void ack (int);

int main (int argc, char** argv)
{
    char sbuffer[max];
    char rbuffer[max];
    int sockfd, filefd;
    int i = 0;
    int n;
    unsigned int sended = 0;
    struct hostent* host;
    struct sockaddr_in server;
    socklen_t len;
    struct stat fin;
    struct packet pac;

    if (argc != 4)
    {
        cout << "Usage: ./sender2 <receiver IP> <receiver port> <file name>" << endl;
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

    // open file 
    filefd = open(argv[3], O_RDONLY);
    if (filefd < 0)
    {
        perror("file open error");
        exit(1);
    }
    fstat(filefd, &fin);

    // send file name to receiver
    signal(SIGALRM, ack);
    siginterrupt(SIGALRM, 1);
    memset(sbuffer, 0, sizeof(sbuffer));
    snprintf(sbuffer, sizeof(sbuffer), "%s", argv[3]);
    while(resend)
    {
        if(sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            perror("send error");
            exit(1);
        }
        resend = false;
        alarm(1);
        while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) <= 0)
        {
            if (resend)
                break;
            perror("recv error");
            exit(1);
        }
        alarm(0);
    }

    // start to send file
    i = 0;
    sended = 0;
    while(sended < fin.st_size)
    {
        memset(&pac, 0, sizeof(pac));
        n = read(filefd, pac.index, sizeof(pac.index));
        sended += n;
        cout << "send: " << sended << endl;
        pac.no = i++;
        pac.size = n;
        
        resend = true;
        while(resend)
        {
            if(sendto(sockfd, &pac, sizeof(pac), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
            {
                perror("send error");
                exit(1);
            }
            resend = false;
            alarm(1);
            while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) <= 0)
            {
                if (resend)
                    break;
                perror("recv error");
                exit(1);
            }
            alarm(0);
        }
    }

    // end send;
    memset(sbuffer, 0, sizeof(sbuffer));
    snprintf(sbuffer, sizeof(sbuffer), "EOF");
    resend = true;
    while(resend)
    {
        if(sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
        {
            perror("send error");
            exit(1);
        }
        resend = false;
        alarm(1);
        while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) <= 0)
        {
            if (resend)
                break;
            perror("recv error");
            exit(1);
        }
        alarm(0);
    }

    cout << "send complete, total packet = " << i-1 << endl;

    close(filefd);
    return 0;
}

void ack (int sig)
{
    resend = true;
    return;
}