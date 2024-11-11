#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <termios.h>


#define MAX_BUFFER_LEN 100
#define MAX_RECV_LEN 1000

int registerUser(int sockfd);
int mainLoop(int sockfd, int sockfd_peer, char *username,
             struct fd_set fds,
             struct timeval timeout);
int printHome();
int homeworkPhase;
int main(int argc, char *argv[])
{
    char in_msg[MAX_BUFFER_LEN] = {0};
    //socket的建立
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
    }
    else
    {
        // recv(sockfd, in_msg, sizeof(in_msg), 0);
        // printf("%s", in_msg);
    }

    //socket的連線
    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = PF_INET;
    char server_addr[20] = "127.0.0.1";
    int port = 6060;
    if (argc >= 2)
    {
        strcpy(server_addr, argv[1]);
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    if (argc >= 4)
    {
        homeworkPhase = atoi(argv[3]);
    }

    info.sin_addr.s_addr = inet_addr(server_addr);
    info.sin_port = htons(port);

    int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
    if (err == -1)
    {
        printf("Connection error");
        return 0;
    }
    else
    {
        recv(sockfd, in_msg, sizeof(in_msg), 0);
        printf("%s", in_msg);
    }

    int sockfd_peer = 0;
    sockfd_peer = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd_peer == -1)
    {
        printf("Fail to create a socket.");
    }

    char username[MAX_BUFFER_LEN] = {0};
    printHome();
    struct fd_set fds;
    struct timeval timeout = {0, 0};
    while (1)
    {
        mainLoop(sockfd, sockfd_peer, username, fds, timeout);
    }

    return 0;
}

int printHome()
{
    printf("======================================\n");
    printf("Please enter the number of service: \n");
    printf("0: Register new account\n");
    printf("1: Login\n");
    printf("2: List account balance and online users\n");
    printf("3: Micropayment transaction\n");
    printf("4: Exit\n\n");
    printf("> ");
    fflush(stdout);
}
int listenOn(int sockfd_peer, int portNo)
{
    // socket的連線
    struct sockaddr_in myInfo;
    int addrlen = sizeof(myInfo);
    bzero(&myInfo, sizeof(myInfo));

    myInfo.sin_family = PF_INET;
    myInfo.sin_addr.s_addr = INADDR_ANY;
    myInfo.sin_port = htons(portNo);
    bind(sockfd_peer, (struct sockaddr *)&myInfo, sizeof(myInfo));
    listen(sockfd_peer, 1);
}

int mainLoop(int sockfd, int sockfd_peer, char *username,
             struct fd_set fds,
             struct timeval timeout)
{
    char action;
    int online_cnt = 0;
    char *userlist;

    FD_ZERO(&fds);
    FD_SET(sockfd_peer, &fds);
    FD_SET(fileno(stdin), &fds);
    int maxfdp = (sockfd_peer > fileno(stdin)) ? sockfd_peer + 1 : fileno(stdin) + 1;
    char buf[MAX_BUFFER_LEN];
    switch (select(maxfdp, &fds, NULL, NULL, &timeout))
    {
    case -1:
        logout(sockfd);
        break;
    case 0:
        break;
    default:
        if (FD_ISSET(sockfd_peer, &fds))
        {
            struct sockaddr_in peerInfo;
            int addr_len = sizeof(peerInfo);
            int forClientSockfd = accept(sockfd_peer, (struct sockaddr *)&peerInfo, &addr_len);
            char peer_msg[MAX_BUFFER_LEN] = {0};
            char recv_msg[MAX_RECV_LEN] = {0};
            recv(forClientSockfd, peer_msg, sizeof(peer_msg), 0);
            if (strlen(peer_msg) > 0)
            {
                char *command = strtok(peer_msg, "#");
                if (strcmp(command, "TX") == 0)
                {
                    char *user = strtok(NULL, "#");
                    char *amount = strtok(NULL, "#");
                    sprintf(peer_msg, "TX#%s#%s#%s\n", user, amount, username);
                    if (homeworkPhase != 1)
                    {
                        send(sockfd, peer_msg, sizeof(peer_msg), 0);
                        recv(sockfd, recv_msg, sizeof(recv_msg), 0);
                    }
                    else
                    {
                        strcpy(recv_msg, "120 TX_ACCEPT");
                    }
                    if (strcmp(recv_msg, "120 TX_ACCEPT") == 0)
                    {
                        user = strtok(user, "#");
                        amount = strtok(NULL, "#");
                        printf("\b\b[NOTE] Receive %s from %s\n> ", amount, user);
                        send(forClientSockfd, recv_msg, sizeof(recv_msg), 0);
                    }
                    else if (strcmp(recv_msg, "240TX_DENIAL") == 0)
                    {
                        send(forClientSockfd, recv_msg, sizeof(recv_msg), 0);
                    }
                    fflush(stdout);
                }
            }
        }
        else if (FD_ISSET(fileno(stdin), &fds))
        {
            action = getchar();
            switch (action)
            {
            case '0':
                registerUser(sockfd);
                printHome();
                break;
            case '1':
                login(sockfd, sockfd_peer, username);
                printHome();
                break;
            case '2':
                if (strlen(username) == 0)
                {
                    printf("Please log in first!\n");
                    printf("---------------------------\n");
                    printf("[Press any key to continue.]\n");
                    fflush(stdin);
                    fgets(buf, MAX_BUFFER_LEN, stdin);
                    printHome();
                    break;
                }
                list(sockfd, &userlist, 1);
                printHome();
                break;
            case '3':
                if (strlen(username) == 0)
                {
                    printf("Please log in first!\n");
                    printf("---------------------------\n");
                    printf("[Press any key to continue.]\n");
                    fflush(stdin);
                    fgets(buf, MAX_BUFFER_LEN, stdin);
                    printHome();
                    break;
                }
                transaction(sockfd, username);
                printHome();
                break;
            case '4':
                logout(sockfd);
                break;
            default:
                break;
            }
        }
    }
}
int transaction(int sockfd, char *username)
{
    char amount[MAX_BUFFER_LEN] = {0};
    char buf[MAX_BUFFER_LEN] = {0};
    char recv_msg[MAX_RECV_LEN] = {0};
    char *userlist;
    int usercnt = list(sockfd, &userlist, 0);
    int recipient = -1;
    printf("Select the recipient user by index: ");
    scanf("%d", &recipient);
    if (recipient >= 0 && recipient < usercnt)
    {
        char *user = strtok(userlist + recipient * MAX_BUFFER_LEN, "#");
        char *user_addr = strtok(NULL, "#");
        char *user_port = strtok(NULL, "\n");

        printf("Please enter the amount to tranfer: ");
        scanf("%s", &amount);
        sprintf(buf, "TX#%s#%s#%s\n", username, amount, user);

        int sockfd_peer = 0;
        sockfd_peer = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd_peer == -1)
        {
            printf("Fail to create a socket.");
        }
        else
        {
            // recv(sockfd, in_msg, sizeof(in_msg), 0);
            // printf("%s", in_msg);
        }

        //socket的連線
        struct sockaddr_in info;
        bzero(&info, sizeof(info));
        info.sin_family = PF_INET;
        info.sin_addr.s_addr = inet_addr(user_addr);
        info.sin_port = htons(atoi(user_port));
        printf("Transfering to: %s:%s...\n", user_addr, user_port);

        int err = connect(sockfd_peer, (struct sockaddr *)&info, sizeof(info));
        if (err == -1)
        {
            printf("Connection error");
            return 0;
        }
        else
        {
            send(sockfd_peer, buf, sizeof(buf), 0);
            recv(sockfd_peer, recv_msg, sizeof(recv_msg), 0);
            printf("%s\n", recv_msg);
            if (strcmp(recv_msg, "120 TX_ACCEPT") == 0)
            {
                printf("Transaction success!\n");
                printf("---------------------------\n");
                printf("[Press any key to continue.]\n");
                fflush(stdin);
                fgets(buf, MAX_BUFFER_LEN, stdin);
            }
            else if (strcmp(recv_msg, "240 TX_DENIAL") == 0)
            {
                printf("Transaction failed1\n");
                printf("---------------------------\n");
                printf("[Press any key to continue.]\n");
                fflush(stdin);
                fgets(buf, MAX_BUFFER_LEN, stdin);
            }
        }
    }
    return 1;
}
int list(int sockfd, char **userlist, int wait)
{
    char buf[MAX_BUFFER_LEN] = "List";
    char recv_msg[MAX_RECV_LEN] = {0};
    send(sockfd, buf, sizeof(buf), 0);
    recv(sockfd, recv_msg, sizeof(recv_msg), 0);

    char *balance = strtok(recv_msg, "\n");
    char *onlineCount = strtok(NULL, "\n");
    int cnt = atoi(onlineCount);
    *userlist = (char *)malloc((size_t)cnt * MAX_BUFFER_LEN * sizeof(char));
    for (int i = 0; i < cnt; i++)
    {
        char *user = strtok(NULL, "\n");
        strcpy(*userlist + i * MAX_BUFFER_LEN, user);
    }
    printf("Account balance: %s\n", balance);
    printf("Current online: %s users\n", onlineCount);
    for (int i = 0; i < cnt; i++)
    {
        printf("[%d] %s\n", i, *userlist + i * MAX_BUFFER_LEN);
    }

    printf("---------------------------\n");
    if (wait != 0)
    {
        printf("[Press any key to continue.]\n");
        fflush(stdin);
        fgets(buf, MAX_BUFFER_LEN, stdin);
    }
    return cnt;
}
int logout(int sockfd)
{
    char buf[MAX_BUFFER_LEN] = "Exit";
    char recv_msg[MAX_BUFFER_LEN] = {0};
    send(sockfd, buf, sizeof(buf), 0);
    recv(sockfd, recv_msg, sizeof(recv_msg), 0);
    if (strcmp(recv_msg, "Bye\n") == 0 && strlen(recv_msg) != 0)
    {
        printf("See you next time!");
        exit(0);
    }
}
int login(int sockfd, int sockfd_peer, char *username)
{
    char buf[MAX_BUFFER_LEN] = {0};
    char portNo[MAX_BUFFER_LEN] = {0};
    char name[MAX_BUFFER_LEN] = {0};
    char recv_msg[MAX_RECV_LEN] = {0};
    printf("Please enter your username: ");
    scanf("%s", &name);
    printf("Please select a port number (default 6061): ");
    fflush(stdin);
    fgets(portNo, MAX_BUFFER_LEN, stdin);
    char *newline = strchr(portNo, '\n');
    if (newline)
        *newline = 0;
    if (strlen(portNo) == 0)
        strcpy(portNo, "6061");
    sprintf(buf, "%s#%s\n", name, portNo);
    send(sockfd, buf, sizeof(buf), 0);
    recv(sockfd, recv_msg, sizeof(recv_msg), 0);
    if (strlen(recv_msg) < 5)
        return 0;
    char *balance = strtok(recv_msg, "\n");
    if (strcmp(balance, "220 AUTH_FAIL") == 0)
    {
        printf("Login failed! User not registered\n");
        printf("---------------------------\n");
        printf("[Press any key to continue.]\n");
        fflush(stdin);
        fgets(buf, MAX_BUFFER_LEN, stdin);
        return 0;
    }
    else if (strcmp(balance, "230 LOGGED_IN") == 0 || strcmp(balance, "This account has been logged in!") == 0)
    {
        printf("Login failed! User is already logged in from other place\n");
        printf("---------------------------\n");
        printf("[Press any key to continue.]\n");
        fflush(stdin);
        fgets(buf, MAX_BUFFER_LEN, stdin);
        return 0;
    }
    char *onlineCount = strtok(NULL, "\n");
    printf("Welcome %s!\n", name);
    printf("Account balance: %s\n", balance);
    printf("Current online: %s users\n", onlineCount);
    strcpy(username, name);
    listenOn(sockfd_peer, atoi(portNo));
    return 1;
}
int registerUser(int sockfd)
{
    char buf[MAX_BUFFER_LEN] = {0};
    char name[MAX_BUFFER_LEN] = {0};
    char recv_msg[MAX_RECV_LEN] = {0};
    int deposit = 0;
    printf("Please enter username: ");
    scanf("%s", &name);
    printf("How much do you want to deposit? ");
    scanf("%d", &deposit);
    sprintf(buf, "REGISTER#%s#%d\n", name, deposit);
    send(sockfd, buf, sizeof(buf), 0);

    recv(sockfd, recv_msg, sizeof(recv_msg), 0);
    char *response_no = strtok(recv_msg, " ");
    if (strcmp(response_no, "100") == 0)
    {
        printf("User %s registered successfully!\n", name);
        return 1;
    }
    else if (strcmp(response_no, "210") == 0)
    {
        printf("Register failed!\n");
        return 0;
    }
    return 0;
}