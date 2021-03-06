#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

#define max 3000
#define rmax 2000

struct packet 
{
    int no;
    char index[rmax];
    unsigned int size;
	const char cmp[] = "sender1";
};

int main (int argc, char** argv)
{
    int sockfd, filefd;
    long sended;
    int i, n;
    char sbuffer[max];
    char rbuffer[rmax];
    struct hostent* host;
    struct sockaddr_in server;
    socklen_t len;
    struct timeval tv;
    struct stat fin;
    struct packet pac;

    if (argc != 4)
    {
        cout << "Usage: ./sender1 <receiver IP> <receiver port> <file name>" << endl;
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        exit(1);
    }

    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("set socket option error");
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
        perror("file doesn't exist");
        exit(1);
    }
    snprintf(sbuffer, sizeof(sbuffer), "%s", argv[3]);
    do
    {
        if (sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
            perror("send error");
    }
    while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0);
    fstat(filefd, &fin);

    // start to send file
    i = 1;
    sended = 0;
    while(sended<fin.st_size)
    {
        memset(&pac, 0, sizeof(struct packet));
        memset(rbuffer, 0, sizeof(rbuffer));
        memset(sbuffer, 0, sizeof(sbuffer));
        n = read(filefd, pac.index, sizeof(rbuffer));
        sended += n;
        cout << "send: " << sended << endl;
        pac.no = i++;
        pac.size = n;
        do
        {
            if (sendto(sockfd, &pac, sizeof(struct packet), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
                perror("send error");
        }
        while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0);
    }

    // end send
    snprintf(sbuffer, sizeof(sbuffer), "EOF");
    do
    {
        if (sendto(sockfd, sbuffer, sizeof(sbuffer), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
            perror("send error");
    }
    while(recvfrom(sockfd, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&server, &len) < 0);

    cout << "send complete, total packet = " << i-1 << endl;

    close(filefd);
    return 0;
}
