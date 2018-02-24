#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
using namespace std;

#define max 1500

int main (int argc, char** argv)
{
	int chk;
	int sockfd;
	char recv_buffer[max];
	char send_buffer[max];
	struct sockaddr_in server;
	struct hostent *host;
	fd_set fileset;

	if (argc != 3)
	{
		cout << "Usage: " << argv[0] << " <SERVER ADDRESS> <SERVER PORT>" << endl;
		exit(1);
	}
	
	host = gethostbyname(argv[1]);
	if (host==NULL)
	{
		cout << "Cannot get host by " << argv[1] << endl;
		exit(1);
	}

	memset(&server,0,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	memcpy(&server.sin_addr, host->h_addr, host->h_length);

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd<0)
	{
		perror("socket error");
		exit(1);
	}

	if (connect(sockfd, (struct sockaddr*)&server, sizeof(server))<0)
	{
		perror("connect error");
		exit(1);
	}

	while(1)
	{
		memset(recv_buffer,0,sizeof(recv_buffer));
		memset(send_buffer,0,sizeof(send_buffer));
		FD_ZERO(&fileset);
		FD_SET(fileno(stdin), &fileset);
		FD_SET(sockfd, &fileset);
		if(select(sockfd+1,&fileset,NULL,NULL,NULL)<0)
		{
			perror("select error");
			exit(1);
		}

		if (FD_ISSET(fileno(stdin), &fileset))
		{
			fgets(send_buffer, sizeof(send_buffer), stdin);
			if (send(sockfd,send_buffer,sizeof(send_buffer),0)<0)
			{
				perror("send error");
				exit(1);
			}
		}

		if (FD_ISSET(sockfd, &fileset))
		{
			if ((chk = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0))<=0)
			{
				if (!chk)
				{
					cout << "The server has closed the connection." << endl;
					close(sockfd);
					break;
				}
				
				if (chk < 0)
				{
					perror("recive error");
					exit(1);
				}
			}
			cout << recv_buffer;
		}
	}
	
	close(sockfd);
	return 0;
}
