#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define size_set 10
#define MAXDATA 1024
#define SERV_PORT 4130
#define LISTENQ 5

struct cli
{
    int fd, port;
    char name[13], ip[20];
};

int str_len(char *tmp)
{
    return strlen(tmp);
}

int valid_name(char *new_name, struct cli client[size_set], int s)
{
    //new name is anonymous
    if(new_name[0] == 'A' || new_name[0] == 'a' && new_name[1] == 'N' || new_name[1] == 'n' && new_name[2] == 'O' || new_name[2] == 'o' && \
       new_name[3] == 'N' || new_name[3] == 'n' && new_name[4] == 'Y' || new_name[4] == 'y' && new_name[5] == 'M' || new_name[5] == 'm' && \
       new_name[6] == 'O' || new_name[6] == 'o' && new_name[7] == 'U' || new_name[7] == 'u' && new_name[8] == 'S' || new_name[8] == 's')
        return 1;

    //new name is exist already
    for(int k = 0; k < size_set; k++)
    {
        if(k != s && strcmp(new_name, client[k].name) == 0)
            return 2;
    }

    //new name is not 2~12 English letters
    if(strlen(new_name) < 2 || strlen(new_name) > 12)
        return 3;
    for(int k = 0; k < strlen(new_name); k++)
    {
        if(new_name[k] < 'A' || new_name[k] > 'z' || (new_name[k] >= 91 && new_name[k] <= 96)) //65 = A, 172 = z, 91~96 are special symbol
            return 3;
    }

    return 0;
}

int valid_receiver(char *recv, struct cli client[size_set])
{
    //receiver is anonymous
    if(strcmp(recv, "anonymous") == 0)
        return 1;

    //receiver is not exist
    if(recv == NULL)
        return 2;

    int k;
    for(k = 0; k < size_set; k++)
    {
        if(strcmp(recv, client[k].name) == 0)
            break;
    }
    if(k == size_set)
        return 2;

    return 0;
}

int main(int argc, char **argv)
{
    int i, maxfd;
    int listenfd, connfd, sockfd;
    int nready, num_client;
    struct cli client[size_set];
    int n;
    fd_set readset, allset;
    socklen_t chilen;
    char recv_data[MAXDATA], send_data[MAXDATA];
    struct sockaddr_in cliaddr, servaddr;

    //== initialize ==
    printf("building a socket....\n");
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("ERROR in opening socket"); //because of socket() can change the errno
        exit(1);
    } //construct a socket to communicate with client

    printf("initializing server information....\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERV_PORT);

    printf("binding....\n");
    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("ERROR in bind()");
        exit(1);
    } //tie the port and local machine together
    printf("listening for socket....\n");
    if(listen(listenfd, LISTENQ) == -1)
    {
        perror("ERROR in listen()");
        exit(1);
    }//set at most how many client can connect to the server

    num_client = 0;
    maxfd = listenfd; //the max fd in fd set
    for(i = 0; i < size_set; i++)
    {
        client[i].fd = -1;
        strcpy(client[i].name, "anonymous");
    }
    FD_ZERO(&allset);
    FD_ZERO(&readset);
    FD_SET(listenfd, &allset);
    //=================
    for(;;)
    {
        bzero(&recv_data, MAXDATA);
        bzero(&send_data, MAXDATA);
        readset = allset;
        nready = select(maxfd+1, &readset, NULL, NULL, NULL);
        if(nready == -1)
        {
            perror("ERROR in select()");
            exit(1);
        }
        printf("select success\n");
        for(i = 0; i <= maxfd; i++)
        {
            if(FD_ISSET(i, &readset))
            {
                if(i == listenfd) // new client want to connect
                {
                    printf("listenfd has heard a new client...\n");
                    clilen = sizeof(cliaddr);
                    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

                    if(connfd == -1)
                        perror("ERROR in accept");
                    else
                    {
                        printf("new client accept success\n");
                        if(num_client >= size_set) // too many clients
                        {
                            fprintf(stderr, "too many clients that new client connect failed");
                            strcpy(send_data, "ERROR: failed to connect because there is too many clients\n");
							send(connfd, send_data, str_len(send_data),0);
							close(connfd);
                        }
                        else //connect success
                        {
                            printf("new client is connecting...\n");
                            FD_SET(connfd, &allset); //put the new client socket's descriptor in the fd set
                            ++num_client;
                            if(connfd > maxfd)
                                maxfd = connfd;

                            char msg_newcli[50];
                            bzero(&msg_newcli, 50);
                            strcpy(msg_newcli, "[Server] Someone is coming!");
                            char msg_hello[100];
                            bzero(&msg_hello, 100);

                            //update the information of client
                            client[connfd].fd = connfd;
                            inet_ntop(AF_INET, &(cliaddr.sin_addr), client[connfd].ip, sizeof(client[connfd].ip));
                            client[connfd].port = cliaddr.sin_port;

                            sprintf(msg_hello, "[Server] Hello, anonymous! From: %s/%d\n", client[connfd].ip, client[connfd].port);
                            printf("sending hello msg....\n");
                            for(int k = 0; k <= maxfd; k++)
                            {
                                if(FD_ISSET(k, &allset))
                                {
                                    if(k == connfd)
                                    {
                                        n = send(connfd, msg_hello, str_len(msg_hello), 0);
                                        if(n <= 0)
                                            printf("hello msg sent failed\n");
                                    }
                                    else if(k != listenfd)
                                        send(k, msg_newcli, str_len(msg_newcli), 0);
                                }
                            }
                            printf("new client connected success!\n");
                        }
                    }
                }
                else //exist client request
                {
                    int n;
                    sockfd = i;
                    n = recv(sockfd, recv_data, sizeof(recv_data), 0);

                    if(n <= 0) //client close connection
                    {
                        num_client--;
                        sprintf(send_data, "[Server] %s if offline\n", client[sockfd].name);
                        for(int k = 0; k <= maxfd; k++)
                        {
                            if(FD_ISSET(k, &allset))
                            {
                                if(k != listenfd && k != sockfd)
                                    send(k, send_data, sizeof(send_data), 0);
                            }
                        }
                        printf("%s is offline\n", client[sockfd].name);
                        client[sockfd].fd = -1;
                        strcpy(client[sockfd].name, "anonymous");
                        close(sockfd);
                        FD_CLR(sockfd, &allset);
                    }
                    else //exist client send a command
                    {
                        printf("%s -> %s\n", client[sockfd].name, recv_data);
                        char *command;
                        command = strtok(recv_data, "\n\r"); //take the string before a space or enter

                        if(strcmp(command, "who") == 0)
                        {
                            for(int k = 0; k < size_set; k++)
                            {
                                if(client[k].fd != -1)
                                {
                                    if(k == sockfd)
                                        sprintf(send_data, "[Server] %s %s/%d ->me\n", client[k].name, client[k].ip, client[k].port);
                                    else
                                        sprintf(send_data, "[Server] %s %s/%d\n", client[k].name, client[k].ip, client[k].port);
                                    send(sockfd, send_data, sizeof(send_data), 0);
                                }
                            }
                        }
                        else if(strcmp(command, "name") == 0)
                        {
                            char old_name[13];
                            char *tmp;
                            bzero(&old_name, 13);
                            strcpy(old_name, client[sockfd].name);

                            tmp = strtok(NULL, "\n\r");
                            int check = valid_name(tmp, client, sockfd);
                            if(check == 0) //valid name
                            {
                                strcpy(client[sockfd].name, tmp);
                                //reply the client who call "name"
                                sprintf(send_data, "[Server] You're now known as %s.\n", client[sockfd].name);
                                send(sockfd, send_data, sizeof(send_data), 0);
                                //broadcast to others client
                                sprintf(send_data, "[Server] %s is now known as %s.\n", old_name, client[sockfd].name);
                                for(int k = 0; k < maxfd; k++)
                                {
                                    if(FD_ISSET(k, &allset))
                                    {
                                        if(k != sockfd && k != listenfd)
                                            send(k, send_data, sizeof(send_data), 0);
                                    }
                                }
                            }
                            else if(check == 1) //new name is anonymous
                            {
                                strcpy(send_data, "[Server] ERROR: Username cannot be anonymous.\n");
                                send(sockfd, send_data, sizeof(send_data), 0);
                            }
                            else if(check == 2) //new name exist already
                            {
                                strcpy(send_data, "[Server] ERROR: <NEW USERNAME> has been used by others.\n");
                                send(sockfd, send_data, sizeof(send_data), 0);
                            }
                            else if(check == 3) //new name is not 2-12 English letters
                            {
                                strcpy(send_data, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
                                send(sockfd, send_data, sizeof(send_data), 0);
                            }
                        }
                        else if(strcmp(command, "tell") == 0)
                        {
                            //anonymous want to send a message
                            if(strcmp(client[sockfd].name, "anonymous") == 0)
                            {
                                strcpy(send_data, "[Server] ERROR: You are anonymous.\n");
                                send(sockfd, send_data, sizeof(send_data), 0);
                            }
                            else
                            {
                                char *receiver;
                                char *msg;
                                bzero(&receiver, 13);

                                receiver = strtok(NULL, "\n\r");
                                int check = valid_receiver(receiver, client);
                                if(check == 0) //valid receiver
                                {
                                    for(int k = 0; k <= maxfd; k++)
                                    {
                                        if(strcmp(receiver, client[k].name) == 0)
                                        {
                                            msg = strtok(NULL, "\n\r");
                                            sprintf(send_data, "[Server] %s tell you %s\n", client[sockfd].name, msg);
                                            n = send(k, send_data, sizeof(send_data), 0);
                                            if(n > 0)
                                            {
                                                strcpy(send_data, "[Server] SUCCESS: Your message has been sent.\n");
                                                send(sockfd, send_data, sizeof(send_data), 0);
                                            }
                                        }
                                    }
                                }
                                else if(check == 1)
                                {
                                    strcpy(send_data, "[Server] ERROR: The client to which you sent is anonymous.\n");
                                    send(sockfd, send_data, sizeof(send_data), 0);
                                }
                                else if(check == 2)
                                {
                                    strcpy(send_data, "[Server] ERROR: The receiver doesn't exist.\n");
                                    send(sockfd, send_data, sizeof(send_data), 0);
                                }
                            }
                        }
                        else if(strcmp(command, "yell") == 0)
                        {
                            char *msg = strtok(NULL, "\n\r");
                            sprintf(send_data, "[Server] %s yell %s\n", client[sockfd].name, msg);
                            for(int k = 0; k <= maxfd; k++)
                            {
                                if(FD_ISSET(k, &allset))
                                {
                                    if(k != listenfd)
                                        send(sockfd, send_data, sizeof(send_data), 0);
                                }
                            }
                        }
                        else
                        {
                            char msg_errcm[50];
                            bzero(&msg_errcm, 50);
                            strcpy(msg_errcm, "[Server] ERROR: Error command.");
                            send(sockfd, msg_errcm, sizeof(msg_errcm), 0);
                        }
                    }
                }
            }
        }
    }
    close(listenfd);
    return 0;
}
