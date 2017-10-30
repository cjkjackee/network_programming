#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define max 1024
struct user
{
	int sockfd;
	char name[max];
	char ip[max];
	struct user* next;
};

const char* server_msg = "[Server]";
struct user *list = NULL;
fd_set sockset;

void service(int, char*, struct user*);
void addUser(int);
void deletUser(struct user*);

int main (int argc, char**argv)
{
	const char* h_msg = "Hello, anonymous!";
	int sockfd,connfd;
	int port;
	int recv_chk;
	char recv_buffer[max];
	char send_buffer[max];
	struct sockaddr_in address;	
	int setlen = 0;
	fd_set sockset,tmp;
	struct user* now;

	if (argc<2)
	{
		printf("usage: ./server <server port>\n");
		exit(1);
	}
			
	list = (user*)malloc(sizeof(user));
	list->sockfd = -1;
	list->next = list;

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
	
	FD_ZERO(&sockset);
	FD_SET(sockfd,&sockset);
	setlen = sockfd + 1;

	while(1)
	{
		tmp = sockset;

		if (select(setlen,&tmp,NULL,NULL,NULL)<0)
		{
			perror("select error");
			exit(4);
		}
		
		if (FD_ISSET(sockfd,&tmp))
		{
			connfd = accept(sockfd,NULL,NULL);
			if (connfd<0)
			{
				perror("accept error");
				exit(1);
			}
			else
			{
				addUser(connfd);
				FD_SET(connfd,&sockset);
				if (connfd>=setlen)
					setlen = connfd + 1;
				snprintf(send_buffer,sizeof(send_buffer),"%s %s From: %s\n",server_msg,h_msg,list->ip);
				send(connfd,send_buffer,sizeof(send_buffer),0);
				for (now=list->next->next;now!=list->next;now=now->next)
				{
					if (now->sockfd==connfd)
						continue;
					else 
					{
						snprintf(send_buffer,sizeof(send_buffer),"%s Someone is coming!\n",server_msg);
						send(now->sockfd,send_buffer,sizeof(send_buffer),0);
					}
				}
			}
		}

		for (now=list->next;now->next!=list->next;now=now->next)
		{
			if (FD_ISSET(now->next->sockfd,&tmp))
			{
				if((recv_chk=recv(now->next->sockfd,recv_buffer,sizeof(recv_buffer),0))<=0)
				{
					struct user* it;

					snprintf(send_buffer,sizeof(send_buffer),"%s %s is offline.\n",server_msg,now->next->name);
					for (it=list->next->next;it!=list->next;it=it->next)
					{
						if (it==now->next)
							continue;
						else
							send(it->sockfd,send_buffer,sizeof(send_buffer),0);
					}
					close(now->next->sockfd);
					FD_CLR(now->next->sockfd,&sockset);
					deletUser(now);
				}
				else 
					service(now->next->sockfd,recv_buffer,now->next);
			}
		}
	}

	return 0;
}

void service (int connfd, char* buffer, struct user* now)
{
	char send_buff[max];
	char *p;
	struct user* it;

	// command who
	if (!strncmp(buffer,"who",3))
	{
		char tmp[max];
		memset(send_buff,0,sizeof(send_buff));
		for (it=list->next->next;it!=list->next;it=it->next)
		{
			if (it==now)
				snprintf(tmp,sizeof(tmp),"%s %s %s ->me\n",server_msg,it->name,it->ip);
			else 
				snprintf(tmp,sizeof(tmp),"%s %s %s\n",server_msg,it->name,it->ip);
			strcat(send_buff,tmp);
		}
	}
	// change user name
	else if (!strncmp(buffer,"name",4))
	{
		int i=-1;
		char name[30];
		char old[30];

		p = strstr(buffer,"name");
		p += 5*sizeof(char);
		snprintf(name,strlen(p),"%s",p);
		i = atoi(name);	
		strncpy(old,now->name,sizeof(now->name));
		// new name is anonymous.
		if (!strcmp(name,"anonymous"))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: Username cannot be anonymous.\n",server_msg);
		// new name does not consist of 2~12 English letters.
		else if (strlen(name)<2 || strlen(name)>12 || (strlen(name)==2 && i>0))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: Username can only consists of 2~12 English letters\n",server_msg);
		else
		{
			// new name is unique or not
			for (it=list->next->next;it!=list->next;it=it->next)	
			{
				if (!strcmp(it->name,name))
				{
					snprintf(send_buff,sizeof(send_buff),"%s ERROR: %s has been used by others.\n",server_msg,name);
					send(connfd,send_buff,strlen(send_buff)+1,0);
					return;			
				}
			}
			// success to change
			snprintf(send_buff,sizeof(send_buff),"%s You're now know as %s\n",server_msg,name);
			snprintf(now->name,sizeof(name),"%s",name);
			send(connfd,send_buff,strlen(send_buff)+1,0);
			snprintf(send_buff,sizeof(send_buff),"%s %s is now known as %s.\n",server_msg,old,name);
			for (it=list->next->next;it!=list->next;it=it->next)
			{
				if (it==now)
					continue;
				else
					send(it->sockfd,send_buff,strlen(send_buff)+1,0);
			}
			return;
		}
	}
	// private message
	else if (!strncmp(buffer,"tell",4))
	{
		char *reciver;

		p = strstr(buffer,"tell");
		p += 5*sizeof(char);
		reciver = strtok(p," ");
		p = strtok(NULL,"");
		if (!strcmp(reciver,"anonymous"))
			snprintf(send_buff,sizeof(send_buff),"%s ERROR: The client to which you sent is anonymous.\n",server_msg);
		else 
		{
			for (it=list->next->next;it!=list->next;it=it->next)
			{
				if (!strcmp(reciver,it->name))
				{
					snprintf(send_buff,sizeof(send_buff),"%s %s tell you %s",server_msg,now->name,p);
					send(it->sockfd,send_buff,sizeof(send_buff),0);
					break;
				}
			}
			snprintf(send_buff,sizeof(send_buff),"%s SUCCESS: Your message has been sent.\n",server_msg);
		}
	}
	// TODO: broadcast message
	else if (!strncmp(buffer,"yell",4))
	{
		snprintf(send_buff,sizeof(send_buff),"%s %s %s",server_msg,now->name,buffer);
		for (it=list->next->next;it!=list->next;it=it->next)
		{
			send(it->sockfd,send_buff,strlen(send_buff)+1,0);
		}
		return;
	}
	// error message
	else 
		snprintf(send_buff,sizeof(send_buff),"%s ERROR: Error command.\n",server_msg);

	send(connfd,send_buff,strlen(send_buff)+1,0);
	return ;
}

void addUser(int fd)
{
	struct user* comming;
	struct sockaddr_in guest;
	char clientIP[20];
	int port;
	socklen_t guest_len;

	guest_len = sizeof(guest);
	getpeername(fd,(struct sockaddr*)&guest,&guest_len);
	inet_ntop(AF_INET,&guest.sin_addr, clientIP,20);
	port = ntohs(guest.sin_port);
	comming = (struct user*)malloc(sizeof(struct user));
	comming->sockfd = fd;
	strncpy(comming->name,"anonymous",9);
	snprintf(comming->ip,sizeof(comming->ip),"%s/%d",clientIP,port);
	comming->next = list->next;
	list->next = comming;
	list = list->next;
	return ;
}

void deletUser(struct user* now)
{
	struct user* del = now->next;
	if (list == del)
		list = now;
	now->next = del->next;
	FD_CLR(del->sockfd,&sockset);
	free(del);
	return;
}
