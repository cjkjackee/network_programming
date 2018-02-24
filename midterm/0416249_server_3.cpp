#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define max 1500

vector<int> user;

bool cal(int);

int main (int argc, char** argv)
{
	bool del;
	int listenfd, connfd;
	int port;
	int max_len;
	struct sockaddr_in address;
	fd_set fileset, tmp;

	signal(SIGCHLD,SIG_IGN);

	if (argc!=2)	
	{
		cout << "Usage: " << argv[0] << " <PORT>" << endl;
		exit(1);
	}

	listenfd = socket(AF_INET,SOCK_STREAM,0);
	memset(&address,0,sizeof(address));
	address.sin_family = AF_INET;
	port = atoi(argv[1]);
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&address, sizeof(address))<0)
	{
		perror("bind error");
		exit(1);
	}

	if (listen(listenfd,15)<0)
	{
		perror("listen error");
		exit(1);
	}
	
	FD_ZERO(&fileset);
	FD_SET(listenfd, &fileset);
	max_len = listenfd + 1;
	while(1)	
	{
		tmp = fileset;
		for(int i=0;i<user.size();++i)
			FD_SET(user[i],&fileset);

		if (select(max_len,&tmp,NULL,NULL,NULL)<0)
		{
			perror("select error");
			exit(1);
		}

		if (FD_ISSET(listenfd, &tmp))
		{
			connfd = accept(listenfd,NULL,NULL);
			if (connfd<0)
			{
				perror("accept error");
				exit(1);
			}
			if (connfd>=max_len)
				max_len = connfd + 1;
			user.push_back(connfd);
			FD_SET(connfd, &fileset);
		}

		for (vector<int>::iterator it=user.begin();it!=user.end();++it)		
		{
			if (FD_ISSET(*it, &tmp))
			{
				del = cal(*it);
				if(del)
				{
					FD_CLR(*it, &fileset);
					close(*it);
					it = user.erase(it);
				}
				if (it==user.end())
					break;
			}
		}

	}
	
	return 0;
}

bool cal(int connfd)
{
	bool over = false;
	char send_buffer[max];
	char recv_buffer[max];
	string s;
	stringstream ss;
	int chk;	
	unsigned long long num;
	unsigned long long tmp;
	unsigned int ans;
	
	over = false;
	memset(send_buffer,0,sizeof(send_buffer));
	memset(recv_buffer,0,sizeof(recv_buffer));
	chk = recv(connfd, recv_buffer, sizeof(recv_buffer), 0);
	if (chk < 0)
	{
		perror("recive error");
		exit(1);
	}
	if (!chk)	
		return true;
	if (!strncmp(recv_buffer,"EXIT",4))
		return true;
	s = recv_buffer;
	ss << s;
	ss >> s;
	if (s=="ADD")
	{
		num = 0;
		while (ss>>tmp)			
			num += tmp;	
		ans = num;
		if (ans!=num)
			over = true;
	}
	else if (s=="MUL")
	{
		num = 1;
		while(ss>>tmp)
			num *= tmp;
		ans = num;
		if (ans!=num)
			over = true;
	}
	else 
		snprintf(send_buffer,sizeof(send_buffer),"error command\n");
	
	if (over)
		snprintf(send_buffer, sizeof(send_buffer), "Overflowed\n");
	else
		snprintf(send_buffer,sizeof(send_buffer),"%u\n",ans);

	send(connfd, send_buffer, sizeof(send_buffer),0);	

	return false;
}
