#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;

#define max 3000
#define pac_max 2000

struct packet
{
	int no;
	char index[2000];
	unsigned int size;
};

int main (int argc, char** argv)
{
	map<int,packet> file;
    int connfd, filefd;
    int l;
    char buffer[max];
    struct sockaddr_in address, guest;
    socklen_t glen = sizeof(guest);
	struct packet pac;

    if (argc != 2)
    {
        cout << "Usage: ./receiver1 <port>" << endl;
        exit(1);
    }

    connfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (connfd < 0)
    {
        perror("socket error");
        exit(1);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(atoi(argv[1]));

    if ( bind(connfd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind error");
        exit(1);
    }

	while (1)
	{
		file.clear();
		memset(buffer,0,sizeof(buffer));
		if ( (l = recvfrom(connfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&guest, &glen)) < 0)
		{
			perror("receive error");
			exit(1);
		}
		buffer[l] = 0;

		if ( sendto(connfd, "1\n", sizeof("1\n"), 0, (struct sockaddr*)&guest,glen) < 0)
		{
			perror("send error");
			exit(1);
		}

		filefd = open(buffer, O_RDWR|O_TRUNC|O_APPEND|O_CREAT,0666);
		cout << "start to transfer " << buffer << endl;

		while(1)
		{
			bzero(buffer,sizeof(buffer));
			if ( (l = recvfrom(connfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&guest, &glen)) < 0)
			{
				perror("receive error");
				exit(1);
			}

			if(!strcmp(buffer,"EOF"))
				break;
			else
			{
				memset(&pac, 0, sizeof(struct packet));
				memcpy(&pac,buffer,sizeof(struct packet));
				if (file.find(pac.no)==file.end())
				{
					file[pac.no] = pac;
					cout << "receive success, no:" << pac.no << endl;
				}
			}
			
			if ( sendto(connfd, "1\n", sizeof("1\n"), 0, (struct sockaddr*)&guest,glen) < 0)
			{
				perror("send error");
				exit(1);
			}
		}

		if ( sendto(connfd, "1\n", sizeof("1\n"), 0, (struct sockaddr*)&guest,glen) < 0)
		{
			perror("send error");
			exit(1);
		}

		for(int x=1;x<=file.size();++x)
			write(filefd, file[x].index, file[x].size);
			
		cout << "file receive complete!" << endl;
		close(filefd);
	}
    return 0;
}
