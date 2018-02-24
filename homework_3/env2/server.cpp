#include <iostream>
#include <fstream>
#include <queue>
#include <set>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;

#define max 1000
#define s_exit -1
#define s_init 0
#define s_update 1
#define s_upload 2
#define s_download 3
#define s_working 4

struct packet
{
    char name[max];
    int state;
    unsigned int no;
    char index[max];
	int length;
};

struct fdata
{
	char index[max];
	int length;
};

struct errpac
{
    int connfd;
    struct packet pac;
};

struct user
{
    set<int> online;
    unsigned int total_pac;
    string file_name;
    map<int, struct fdata> file;
    map<string,bool> list;   
};

queue<struct errpac> errlist;
map<string, struct user> client;
fd_set fileset;

void service (struct packet&, int);

int main (int argc, char** argv)
{
    int flag;
    int sockfd, connfd;
    struct packet spac, rpac;
    struct sockaddr_in address;
    int setlen = 0;
    map<string,struct user>::iterator it;
    fd_set tmpset;

    client.clear();

    if (argc != 2)
    {
        cout << "Usage: ./server <port>" << endl;
        exit(1);
    }

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (sockfd < 0)
    {
        perror("socket error");
        exit(1);
    }

    flag = fcntl(sockfd,F_GETFL,0);
    fcntl(sockfd,F_SETFL, flag|O_NONBLOCK);

    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        perror("bind error");
        exit(1);
    }

    if (listen(sockfd,10)<0)
    {
        perror("listen error");
        exit(1);
    }

    FD_ZERO(&fileset);
    FD_SET(sockfd,&fileset);
    setlen = sockfd + 1;
    
    while(1)
    {
        for (int i=errlist.size();i<0;--i)
        {
            if (send(errlist.front().connfd, &errlist.front().pac, sizeof(errlist.front().pac),0)<0)
            {
                if (errno == EWOULDBLOCK)
                {
                    errlist.push(errlist.front());
                    continue;
                }
                perror("send error");
                exit(1);
            }
            errlist.pop();
        }

        tmpset = fileset;
        select(setlen,&tmpset,NULL,NULL,NULL);

        if (FD_ISSET(sockfd,&tmpset))
        {
            struct user init;

            connfd = accept(sockfd,NULL,NULL);
            if (connfd<0)
            {
                perror("accept error");
                exit(1);
            }
            

            fcntl(connfd,F_SETFL,O_NONBLOCK);

            // receive the username from client
            memset(&rpac,0,sizeof(rpac));
            while (recv(connfd,&rpac,sizeof(rpac),0)<0 && errno==EWOULDBLOCK) 
            {
                if (errno!=EWOULDBLOCK)
                {
                    perror("receive error");
                    exit(1);
                }
            }

            memset(&init,0,sizeof(init));
            it = client.find(rpac.name);
            // if the client is a new user, creat a data struct to save it infor
            if (it==client.end())
            {
                client[rpac.name] = init;
                spac.state = s_init;
                spac.no = 1;
                if(send(connfd,&spac,sizeof(spac),0) < 0)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("send error");
                        exit(1);
                    }
                }
            }
            // if the client is not a new user, send the file list to client
            else
            {
                memset(&spac,0,sizeof(spac));
                memcpy(&spac.name,&rpac.name,sizeof(rpac.name));
                spac.state = s_init;
                spac.no = 0;
                for (map<string,bool>::iterator it=client[rpac.name].list.begin();it!=client[rpac.name].list.end();++it)
                {
                    snprintf(spac.index,sizeof(spac.index),"%s",it->first.c_str());

                    if(send(connfd,&spac,sizeof(spac),0) < 0)
                    {
                        if (errno == EWOULDBLOCK)
                            continue;
                        perror("send error");
                        exit(1);
                    }
                }
                spac.no = 1;
                if(send(connfd,&spac,sizeof(spac),0) < 0)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("send error");
                        exit(1);
                    }
                }
            }

            // store the fd
            client[rpac.name].online.insert(connfd);
            FD_SET(connfd,&fileset);
            if (connfd>=setlen)
                setlen = connfd + 1;
        }

        for (it=client.begin();it!=client.end();++it)
        {
            struct user u = it->second;
            for (set<int>::iterator it2=u.online.begin();it2!=u.online.end();++it2)
            {
                if (FD_ISSET(*it2,&tmpset))
                {
                    memset(&rpac,0,sizeof(rpac));
                    while (recv(*it2,&rpac,sizeof(rpac),0)<0 && errno==EWOULDBLOCK) 
                    {
                        if (errno!=EWOULDBLOCK)
                        {
                            perror("receive error");
                            exit(1);
                        }
                    }
                    service(rpac,*it2);
                }
            }
        }
    }

    return 0;
}

void service (struct packet& pac, int connfd)
{
    struct errpac epac;
    struct packet spac;
	struct fdata data;
    fstream file;
    set<int>::iterator it;

    if (pac.state == s_upload)
    {
        client[pac.name].total_pac = pac.no;
        client[pac.name].file_name = pac.index;
        client[pac.name].file.clear();
    }
    else if (pac.state == s_download)
    {
        file.open(pac.index,fstream::in);

        // get length of file
        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);
        
        int total_pac = length/max;
        if (length%max!=0)
            ++total_pac; 

        cout << total_pac << endl;
        
        // tell client the total packet of the file
        memset(&spac,0,sizeof(spac));
        memcpy(&spac,&pac,sizeof(pac));
        spac.no = total_pac;
        if(send(connfd,&spac,sizeof(spac),0) < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                epac.connfd = connfd;
                epac.pac = spac;
                errlist.push(epac);
            }
            perror("send error");
            exit(1);
        }
        
        // sending file 
        memset(&spac,0,sizeof(spac));
        memcpy(&spac,&pac,sizeof(pac));
        spac.state = s_working;
        for (int i=1;i<=total_pac;++i)
        {
            spac.no = i;
            file.read(spac.index, sizeof(spac.index));
			spac.length = file.gcount();
            if (send(connfd, &spac, sizeof(spac), 0) < 0)
            {
                if (errno==EWOULDBLOCK)
                {
                    epac.connfd = connfd;
                    epac.pac = spac;
                    errlist.push(epac);
                    continue;
                }
                perror("send error");
                exit(1);
            }
        }

        file.close();
    }
    else if (pac.state == s_working)
    {
        // store the file data in a map
		memset(&data,0,sizeof(data));
		memcpy(data.index,pac.index,sizeof(pac.index));
		data.length = pac.length;
		client[pac.name].file[pac.no] = data;

        if (pac.no == client[pac.name].total_pac)
        {
            // write out the file
            file.open(client[pac.name].file_name,fstream::out);
            for (unsigned int i=1;i<=client[pac.name].total_pac;++i)
                file.write(client[pac.name].file[i].index,client[pac.name].file[i].length);
            file.close();
            client[pac.name].list[client[pac.name].file_name] = true;
            client[pac.name].file.clear();
            
            // tell all the user to update the file
            memset(&spac,0,sizeof(spac));
            spac.state = s_update;
            snprintf(spac.index,sizeof(spac.index),"%s",client[pac.name].file_name.c_str());
            spac.no = 0;
            for (it=client[pac.name].online.begin();it!=client[pac.name].online.end();++it)
            {
                if(*it==connfd)
                    continue;
                
                if(send(*it,&spac,sizeof(spac),0) < 0)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("send error");
                        exit(1);
                    }
                }
            }
        } 
    }
    else if (pac.state == s_exit)
    {
        FD_CLR(connfd,&fileset);
        it = client[pac.name].online.find(connfd);
        client[pac.name].online.erase(it);
    }

    return;
}
