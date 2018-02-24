#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define max 1500

vector<pid_t> user;

void cal(int);

int main (int argc, char** argv)
{
	int listenfd, connfd;
	int port;
	int status;
	struct sockaddr_in address;
	pid_t pid, end;
	
	signal(SIGCHLD, SIG_IGN);

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

	while(1)	
	{
		connfd = accept(listenfd,NULL,NULL);
		if(connfd<0)
		{
			perror("accept error");
			exit(1);
		}

		pid = fork();
		if (!pid)	
		{
			cal(connfd);
			close(connfd);
			cout << "Client has closed the connection." << endl;
			exit(0);
		}
		else
		{
			user.push_back(pid);
			close(connfd);
		}
	}
	
	return 0;
}

void cal(int connfd)
{
	bool over = false;
	char send_buffer[max];
	char recv_buffer[max];
	string s;
	int chk;	
	unsigned long long num;
	unsigned long long tmp;
	unsigned int ans;
	
	while(1)
	{
		over = false;
		memset(send_buffer,0,sizeof(send_buffer));
		memset(recv_buffer,0,sizeof(recv_buffer));
		chk = recv(connfd, recv_buffer, sizeof(recv_buffer), 0);
		if (!chk || chk<0)	
			break;
		if (!strncmp(recv_buffer,"EXIT",4))
			break;
		s = recv_buffer;
		stringstream ss;
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
		{
			snprintf(send_buffer,sizeof(send_buffer),"error command\n");
		}
		
		if (over)
			snprintf(send_buffer, sizeof(send_buffer), "Overflowed\n");
		else
			snprintf(send_buffer,sizeof(send_buffer),"%u\n",ans);

		send(connfd, send_buffer, sizeof(send_buffer),0);
	}

	return;
}
