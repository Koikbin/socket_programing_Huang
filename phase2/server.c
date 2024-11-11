#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "queue.h"

#define MAX_WORKER 2
#define MAX_BUFFER_LEN 100
#define MAX_ACCT 20
#define MAX_RECV_LEN 1000
struct connection
{
    int sockfd;
    struct sockaddr_in client_addr;
};
struct account
{
    char user[MAX_BUFFER_LEN];
    int balance;
    int logged_in;
    int sess_idx;
};
struct session
{
    
    int acct_idx;
    struct sockaddr_in addr;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t *threadPool[MAX_WORKER];
int acctCount;
struct account *accountList[MAX_ACCT];
int sessionCount;
struct session *sessionList[MAX_ACCT];
queue connectionQueue;
void *execThread(void *);
int main(int argc, char** argv)
{
    connectionQueue.size = 0;
    acctCount = 0;
    sessionCount = 0;
    char inputBuffer[256] = {};
    char welcomeMessage[] = {"Welcome to JHow's server...\n"};
    int sockfd = 0, forClientSockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
    }

    //socket的連線
    struct sockaddr_in serverInfo;
    int addrlen = sizeof(serverInfo);
    bzero(&serverInfo, sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(6060);
    if (argc >= 2)
    {
        serverInfo.sin_port = htons(atoi(argv[1]));
    }
    bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
    listen(sockfd, 5);

    for (int i = 0; i < MAX_WORKER; i++)
    {
        pthread_t *t;
        pthread_create(t, NULL, execThread, (void *)NULL);
        threadPool[i] = t;
    }

    while (1)
    {
        struct connection *clientConnect;
        clientConnect = (struct connection *)malloc(sizeof(*clientConnect));
        forClientSockfd = accept(sockfd, (struct sockaddr *)&clientConnect->client_addr, &addrlen);
        char client_IP[20];
        inet_ntop(AF_INET, &(clientConnect->client_addr.sin_addr), client_IP, INET_ADDRSTRLEN);
        printf("New connection from %s:%d succeeded.\n", client_IP, (int)clientConnect->client_addr.sin_port);
        send(forClientSockfd, (void *)welcomeMessage, sizeof(welcomeMessage), 0);
        clientConnect->sockfd = forClientSockfd;
        push(&connectionQueue, (void *)clientConnect);
        // send(forClientSockfd,welcomeMessage,sizeof(welcomeMessage),0);
        // recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
        // printf("Get:%s\n",inputBuffer);
    }
    return 0;
}

void *execThread(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (connectionQueue.size == 0)
        {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        struct connection *clientConnect = (struct connection *)peek(connectionQueue);
        pop(&connectionQueue);
        pthread_mutex_unlock(&mutex);
        handleConnection(clientConnect);
    }
}

int handleConnection(struct connection *clientConnection)
{
    int sockfd = clientConnection->sockfd;
    struct sockaddr_in client_addr = clientConnection->client_addr;
    int userIdx = -1;

    while (1)
    {
        char inputBuffer[MAX_RECV_LEN] = {0};
        recv(sockfd, (void *)inputBuffer, sizeof(inputBuffer), 0);
        printf("[RECEIVE]\n%s\n", inputBuffer);
        int status = parseRequest(sockfd, &userIdx, client_addr, inputBuffer);
        if (status == 1)
        {
            break;
        }
    }
    return 0;
}

void *showOnline(char *str, int idx)
{
    sprintf(str, "%d\n%d\n", accountList[idx]->balance, sessionCount);
    for (int i = 0; i < sessionCount; i++)
    {
        char line[MAX_BUFFER_LEN];
        struct account acct = *accountList[sessionList[i]->acct_idx];
        char client_IP[20];
        inet_ntop(AF_INET, &(sessionList[i]->addr.sin_addr), client_IP, INET_ADDRSTRLEN);
        sprintf(line, "%s#%s#%d\n", acct.user, client_IP, sessionList[i]->addr.sin_port);
        strcat(str, line);
    }
}

int parseRequest(int sockfd, int *idx, struct sockaddr_in client_addr, char *req)
{
    char receiveMsg[MAX_BUFFER_LEN] = {0};
    char responseBuffer[MAX_RECV_LEN] = {0};
    char req_cpy[MAX_BUFFER_LEN];
    strcpy(req_cpy, req);
    char *command = strtok(req_cpy, "#\n");
    if (strcmp(req, "List") == 0)
    {
        // List
        showOnline(responseBuffer, *idx);
        send(sockfd, (void *)responseBuffer, sizeof(responseBuffer), 0);
        return 0;
    }
    else if (strcmp(req, "Exit") == 0)
    {
        if (*idx != -1)
        {
            accountList[*idx]->logged_in = 0;
            sessionList[accountList[*idx]->sess_idx] = sessionList[sessionCount - 1];
            sessionCount--;
        }
        strcpy(responseBuffer, "Bye\n");
        send(sockfd, (void *)responseBuffer, sizeof(responseBuffer), 0);
        close(sockfd);
        return 1;
        // Exit
    }
    else if (strcmp(command, "TX") == 0)
    {
        char *payer = strtok(NULL, "#");
        int amount = atoi(strtok(NULL, "#"));
        char *payee = strtok(NULL, "\n");
        for (int i = 0; i < acctCount; i++)
        {
            if (strcmp(accountList[i]->user, payer) == 0)
            {
                if (accountList[i]->balance < amount)
                {
                    strcpy(responseBuffer, "240 TX_DENIAL");
                    send(sockfd, responseBuffer, sizeof(responseBuffer), 0);
                    return -1;
                }
                else
                {
                    accountList[i]->balance -= amount;
                }
                break;
            }
        }
        for (int i = 0; i < acctCount; i++)
        {
            if (strcmp(accountList[i]->user, payee) == 0)
            {
                accountList[i]->balance += amount;
                break;
            }
        }
        strcpy(responseBuffer, "120 TX_ACCEPT");
        send(sockfd, responseBuffer, sizeof(responseBuffer), 0);
    }
    else if (strcmp(command, "REGISTER") == 0)
    {
        // Register
        char *username = strtok(NULL, "#");
        char *deposit = strtok(NULL, "\n");
        struct account *newUser = (struct account *)malloc(sizeof(struct account));
        newUser->balance = atoi(deposit);
        strcpy(newUser->user, username);
        newUser->logged_in = 0;
        newUser->sess_idx = -1;
        if (acctCount == MAX_ACCT)
        {
            strcpy(responseBuffer, "210 FAIL\n");
            send(sockfd, responseBuffer, sizeof(responseBuffer), 0);
            return -1;
        }
        accountList[acctCount] = newUser;
        acctCount++;
        strcpy(responseBuffer, "100 OK\n");
        send(sockfd, responseBuffer, sizeof(responseBuffer), 0);
        return 0;
    }
    else
    {
        // Login
        char *username = command;
        int port = atoi(strtok(NULL, "\n"));
        for (int i = 0; i < acctCount; i++)
        {
            if (strcmp(accountList[i]->user, username) == 0 && accountList[i]->logged_in == 0)
            {
                accountList[i]->logged_in = 1;
                *idx = i;
                break;
            }
            else if (strcmp(accountList[i]->user, username) == 0 && accountList[i]->logged_in == 1)
            {
                strcpy(responseBuffer, "230 LOGGED_IN\n");
                send(sockfd, (void *)responseBuffer, sizeof(responseBuffer), 0);
                return -1;
            }
        }
        if (*idx == -1)
        {
            strcpy(responseBuffer, "220 AUTH_FAIL\n");
            send(sockfd, (void *)responseBuffer, sizeof(responseBuffer), 0);
            return -1;
        }
        client_addr.sin_port = (in_port_t)port;
        struct session *newSession = (struct session *)malloc(sizeof(struct session));
        newSession->acct_idx = *idx;
        newSession->addr = client_addr;
        sessionList[sessionCount] = newSession;
        accountList[*idx]->sess_idx = sessionCount;
        sessionCount++;
        showOnline(responseBuffer, *idx);
        send(sockfd, (void *)responseBuffer, sizeof(responseBuffer), 0);
        return 0;
    }

    // Send Response
    return 0;
}