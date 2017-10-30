#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define max 1000
const char* server_msg = "[Server]";

void service(int, char*);

int main (int argc, char**argv)
{
	int sockfd,connfd;
	int port;
	int recv_chk;
	char clientIP[20];
	char* h_msg = "Hello, anonymous!";
	char recv_buffer[max];
	char msg_buffer[max];
	struct sockaddr_in address, guest;	
	socklen_t guest_size;
	pid_t pid;

	if (argc<2)
	{
		printf("usage: ./server <server port>\n");
		exit(1);
	}

	sockfd = socket(AF_INET,SOCK_STREAM,0);	
	memset(&address,0,sizeof(address));
	address.sin_family = AF_INET;
	port = atoi(argv[1]);
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd,(struct sockaddr*)&address,sizeof(address))<0)
	{
		perror ("bind error");
		exit(1);
	}

	if (listen(sockfd,10)<0)
	{
		perror ("listen error");
		exit(1);
	}
	
	while(1)
	{
		connfd = accept(sockfd,(struct sockaddr*)NULL,NULL);
		if (connfd<0)
		{
			perror("accept error");
			exit(1);
		}
		else
			pid = fork();
		if (!pid)
		{
			//show client infor to client himself
			getpeername(connfd,(struct sockaddr*)&guest,&guest_size);
			inet_ntop(AF_INET,&guest.sin_addr, clientIP,20);
			port = ntohs(guest.sin_port);
			guest_size = sizeof(guest);
			snprintf(msg_buffer,max,"%s %s From: %s/%d\n",server_msg,h_msg,clientIP,port);
			send(connfd,msg_buffer,strlen(msg_buffer)+1,0);
			// TODO: broadcast to other online client
			while((recv_chk=recv(connfd,recv_buffer,sizeof(recv_buffer),0)))
			{
				printf("serving my client\n");
				printf("%s\n",recv_buffer);
				if (recv_chk<0)
					perror("recive error");
				service(connfd,recv_buffer);	
				printf("done with my client\n");
				memset(recv_buffer,0,sizeof(recv_buffer));
			}
			if (!recv_chk)
			{
				close(connfd);
				return 0;
			}
			// TODO: broadcast the offline message to other online client
		}
		else
			close(connfd);
	}
	
	return 0;
}

void service (int connfd, char* buffer)
{
	char send_buff[max];
	char *p;

	// TODO: command who
	// change user name
	if (!strncmp(buffer,"name",4))
	{
		int i=-1;
		char name[30];

		p = strstr(buffer,"name");
		p += 5*sizeof(char);
		snprintf(name,strlen(p),"%s",p);
		i = atoi(name);	
		if (!strcmp(name,"anonymous"))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: Username cannot be anonymous.\n",server_msg);
		// TODO: unique name
		else if (strlen(name)<2 || strlen(name)>12 || (strlen(name)==2 && i>0))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: Username can only consists of 2~12 English letters\n",server_msg);
		else 
			snprintf(send_buff,sizeof(send_buff),"%s You're now know as %s\n",server_msg,name);
	}
	// private message
	else if (!strncmp(buffer,"tell",4))
	{
		char *reciver;

		p = strstr(buffer,"tell");
		p += 5*sizeof(char);
		reciver = strtok(p," ");
		p += sizeof(reciver);
		if (!strcmp(reciver,"anonymous"))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: The client to which you sent is anonymous.\n",server_msg);
		else 
			snprintf(send_buff,sizeof(send_buff),"%s SUCCESS: Your message has been sent.\n",server_msg);
	}
	else 
		snprintf(send_buff,sizeof(send_buff),"%s ERROR: Error command.\n",server_msg);

	send(connfd,send_buff,sizeof(send_buff)+1,0);
	return ;
}