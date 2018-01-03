#include <iostream>
#include <fstream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
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

string name;
map<string,bool> list;
map<int, struct fdata> file;

int main (int argc, char** argv)
{
    fstream fin, fout;
    int connfd;
    string cmd;
    struct packet spac, rpac;
	struct fdata data;
    struct hostent* host;
    struct sockaddr_in server;
    fd_set fileset;

    if (argc != 4)
    {
        cout << "Usage: ./client <IP> <PORT> <USERNAME>" << endl;
        exit(1);
    }
    name = argv[3];

    host = gethostbyname(argv[1]);
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr,host->h_addr,host->h_length);
    server.sin_port = htons(atoi(argv[2]));

    connfd = socket(AF_INET,SOCK_STREAM,0);
    if (connfd < 0)
    {
        perror("socket error");
        exit(1);
    }

    if ((connect(connfd,(struct sockaddr*)&server,sizeof(server))) < 0)
    {
        perror("connect error");
        exit(1);
    }

    memset(&spac,0,sizeof(spac));
    spac.state = s_init;
    spac.no = 0;
    snprintf(spac.name,sizeof(spac.name),"%s",argv[3]);
    if (send(connfd,&spac,sizeof(spac),0) < 0)
    {
        perror("send error");
        exit(1);
    }

    while(1)
    {
        memset(&rpac,0,sizeof(rpac));
        if (recv(connfd,&rpac,sizeof(rpac),0) < 0)
        {
            perror("receive error");
            exit(1);
        }
        if (rpac.no)
            break;
        list[rpac.index] = false; 
    }

    cout << "welcome to the dropbox-like server! : " << name << endl;

    while(1)
    {
        for (map<string,bool>::iterator it = list.begin();it!=list.end();++it)
        {
            if (!it->second)
            {
                cout << "Downloading file : " << it->first << endl;
                cout << "Progress : [";
                memset(&spac,0,sizeof(spac));
                snprintf(spac.name,sizeof(spac.name),"%s",name.c_str());
                spac.state = s_download;
                spac.no = 0;
                snprintf(spac.index,sizeof(spac.index),"%s",it->first.c_str());
                if(send(connfd,&spac,sizeof(spac),0)<0)
                {
                    perror("send error");
                    exit(1);
                }

                memset(&rpac,0,sizeof(rpac));
                if (recv(connfd,&rpac,sizeof(rpac),0)<0)
                {
                    perror("receive error");
                    exit(1);
                }
                unsigned int total_pac = rpac.no;
                unsigned int each = 20/total_pac;
                if (total_pac>20)
                    each = total_pac/20;
                fout.open(rpac.index,fstream::out);

                unsigned int total_recv = 0;
                while(total_recv!=total_pac)
                {
                    memset(&rpac,0,sizeof(rpac));
                    if (recv(connfd,&rpac,sizeof(rpac),0)<0)
                    {
                        perror("receive error");
                        exit(1);
                    }    

                    if (rpac.name==name && rpac.state == s_working) 
                    {
                        if (total_pac>20 && total_recv%each==0)
                            cout << "#";
                        else if (total_pac<20)
                        {
                            for (unsigned int x=0;x<each;++x)
                                cout << "#";
                        }

						memset(&data,0,sizeof(data));
						memcpy(data.index,rpac.index,sizeof(rpac.index));
						data.length = rpac.length;
						file[rpac.no] = data;
                        ++total_recv;
                    }
                }
                cout << "]" << endl;

                for (unsigned int i=1;i<=total_pac;++i)
                    fout.write(file[i].index,file[i].length);
                fout.close();
                it->second = true;
            }
        }

        FD_ZERO(&fileset);
        FD_SET(fileno(stdin),&fileset);
        FD_SET(connfd,&fileset);
        select(connfd+1, &fileset, NULL, NULL, NULL);

        // stdin has input && get input
        if (FD_ISSET(fileno(stdin),&fileset))
        {
            cin >> cmd;

            if (cmd == "/exit")
            {
                snprintf(spac.name,sizeof(spac.name),"%s",name.c_str());
                spac.state = s_exit;
                if (send(connfd, &spac, sizeof(spac), 0)<0)
                {
                    perror("send error");
                    exit(1);
                }
                break;
            }

            if (cmd == "/put")
            {
                string operand;

                cin >> operand;

                // open file
                fin.open(operand, fstream::in);
                
                if (!fin)
                {
                    cout << "file open failed" << endl;
                    continue;
                }
                list[operand] = true;

                memset(&spac, 0, sizeof(spac));
                
                // get length of file
                fin.seekg(0, fin.end);
                int length = fin.tellg();
                fin.seekg(0, fin.beg);
                
                int total_pac = length/max;
                if (length%max!=0)
                    ++total_pac;
                int n = 20/total_pac;
                if (total_pac>20)
                    n = total_pac/20;

                // tell server start to receive
                snprintf(spac.name,sizeof(spac.name),"%s",name.c_str());
                spac.state = s_upload;
                snprintf(spac.index, sizeof(spac.index), "%s", operand.c_str());
                spac.no = total_pac;
                if (send(connfd, &spac, sizeof(spac),0)<0)
                {
                    perror("send error");
                    exit(1);
                }

                cout << "Uploading file : " << operand << endl;
                cout << "Progress : [";

                memset(&spac, 0, sizeof(spac));
                snprintf(spac.name,sizeof(spac.name),"%s",name.c_str());
                spac.state = s_working;
                for (int i=1;i<=total_pac;++i)
                {
                    spac.no = i;
                    fin.read(spac.index, sizeof(spac.index));
					spac.length = fin.gcount();
                    if (send(connfd, &spac, sizeof(spac), 0) < 0)
                    {
                        perror("send error");
                        exit(1);
                    }
                    if (total_pac>20 && i%n==0)
                        cout << "#";
                    else if (total_pac<20)
                    {
                        for (int i=0;i<n;++i)
                            cout << "#";
                    }
                }
                cout << "]" << endl;
                fin.close();
            }
            else if (cmd == "/sleep")
            {
                int operand;
                cin >> operand;

                cout << "client starts to sleep" << endl;
                for (int i=1;i<=operand;++i)
                {
                    cout << "sleep " << i << endl;
                    sleep(1);
                }
                cout << "client wakes up" << endl;
                for (map<string,bool>::iterator it = list.begin();it!=list.end();++it)
                    cout << it->first << endl;

            }
            else if (cmd == "/list")
            {
                for (map<string,bool>::iterator it = list.begin();it!=list.end();++it)
                    cout << it->first << endl;
            }
            else 
                cout << "unvalid command" << endl;
        }

        if (FD_ISSET(connfd, &fileset))
        {
            memset(&rpac,0,sizeof(rpac));
            recv(connfd,&rpac,sizeof(rpac),0);
            if (rpac.state == s_update)
                list[rpac.index] = false;
        }
    }
    
    return 0;
}
