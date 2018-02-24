#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define max 1500

map<string,port> user;

int main (int argc, char** argv)
{
	int sockfd,connfd;
	int port;
	int max_len;
	char send_buffer[max];
	char recv_buffer[max];
	string me, name;
	struct sockaddr_in address, client;
	socklen_t client_len;
	fd_set room, tmp;

	if (argc < 3)
	{
		cout << "Usage: " << argv[0] << " <NAME> <PORT> ..." << endl;
		exit(1);
	}
	
	me = argv[1];
	port = atoi(argv[2]);

	for (int i=3;i<argc;++i)
	{	
		memset(send_buffer,0,sizeof(send_buffer));
		memset(recv_buffer,0,sizeof(recv_buffer));
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = (atoi(argv[i]));
		address.sin_addr.s_addr = htonl(INADDR_ANY);	
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (connect(sockfd,(struct sockaddr*)&address,sizeof(address))<0)
		{
			perror("connection error");
			exit(1);
		}

		recv(sockfd,recv_buffer,sizeof(recv_buffer),0);
		name = recv_buffer;
		map[name] = atoi(argv[i]);
		snprintf(send_buffer,sizeof(send_buffer),"%s",me);
		send(sockfd,send_buffer,sizeof(send_buffer),0);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = port;
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind (sockfd, (struct sockaddr*)&address, sizeof(address))<0)
	{
		perror("bind error");
		exit(1);
	}

	if (listen(sockfd,15)<0)
	{
		perror("listen error");
		exit(1);
	}

	FD_ZERO(&room);
	FD_SET(fileno(stdin), &room);
	FD_SET(sockfd, &room);
	max_len = sockfd + 1;

	while(1)
	{
		tmp = room;
		memset(send_buffer,0,sizeof(send_buffer));
		memset(recv_buffer,0,sizeof(recv_buffer));

		if (select(max_len, &tmp, NULL, NULL, NULL)<0)
		{
			perror("select error");
			exit(4);
		}

		if (FD_ISSET(sockfd,&tmp))
		{
			client_len = sizeof(client);
			connfd = accept(sockfd,(sturct sockaddr*)&client, &client_len);
			if (connfd<0)
			{
				perror("accept error");
				exit(1);	
			}
			FD_SET(connfd, &room);
			if (connfd>=max_len)
				max_len = connfd + 1;
		
			snprintf(send_buffer,sizeof(send_buffer),"%s",me);
			send(connfd,send_buffer,sizeof(send_buffer),0);
			
			recv(connfd,recv_buffer,sizeof(recv_buffer),0);
			name = recv_buffer;
			user[name] = ntohs(client.sin_port);
		}

		for (map<int>::iterator it=user.begin();it!=user.end();++it)
		{
			if(FD_ISSET(it->second, &tmp))
			{
				recv(it->second,recv_buffer,sizeof(recv_buffer),0)
				cout << recv_buffer << endl;
			}
		}


	}

	return 0;
}
