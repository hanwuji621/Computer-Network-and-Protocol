/*
 * File: assignment1.c - Implementation of a simple clent-server using the tcp/ip
 * Course: COMP7005  assignment
 * Date: Oct.2 2020
 * Author: Bin Zhu
 *
 */

#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "assignment1.h"


int listenSocket; //listens for incoming connections
int messageSocket;//transfers commands
int dataSocket;   //transfers data

bool isClient = false;  //true: the program is running as the client
bool isServer = false;  //true: the program is running as the server


int main(int argc, char **argv) {
    
    int option;
    // -c : client
    // -s : server
    while ((option = getopt(argc, argv, "cs")) != -1) {
        switch (option) {
            case 'c':
                isClient = true;
                puts("This is the Client!");
                break;
            case 's':
                isServer = true;
                puts("This is the Server!");
                break;
        }
    }
    if (isClient == isServer) {
        puts("Please re-execute the program with below flags");
        puts("-c represents client mode, -s represents server mode");
        return EXIT_SUCCESS;
    }

    /*socket creation*/
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    /*Bind an address to the socket*/
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTENPORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }

    /*queue up to 5 connections*/
    listen(listenSocket, 5);

    if (isServer) {
        runServer();
    } else {
        runClient();
    }
    
    close(listenSocket);

    return EXIT_SUCCESS;
}

/*
 *  runServer: run as the server
 */
void runServer(void) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(struct sockaddr_in);
    memset(&clientAddr, 0, sizeof(struct sockaddr_in));

    //while(true) {

        /*waiting for GET or SEND command */
        if ((messageSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrSize)) == -1) {
            perror("Accept failure");
            exit(EXIT_FAILURE);
        }

        printf("Remote Address: %s\n", inet_ntoa(clientAddr.sin_addr));

        /*initiate a data connection from the port 7005*/
        /*dateSocket creation*/
        if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        clientAddr.sin_port = htons(LISTENPORT);

        if (connect(dataSocket, (struct sockaddr *) &clientAddr, clientAddrSize) == -1) {
            perror("Connection error");
            exit(EXIT_FAILURE);
        }
        /*set messageSocket non blocking*/
        setNonBlocking(messageSocket);

        /*handle 'GET' or 'SEND' in the control channel*/
        handleMessage();

        close(dataSocket);
        close(messageSocket);
    //}
}

/*
 *  FUNCTION: run as the client
 */
void runClient(void) {
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(LISTENPORT);
    
    struct hostent *hp = getDestination();
    memcpy(&server.sin_addr, hp->h_addr_list[0], hp->h_length);

    //for (;;) {
        /*socket creation*/
        if ((messageSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        /*conecting to the server*/
        if (connect(messageSocket, (struct sockaddr *) &server, sizeof(server)) == -1) {
            perror("Connection error");
            exit(EXIT_FAILURE);
        }

        printf("connected to the server: %s successfully\n", hp->h_name);

        /*waiting for date transfer*/
        if ((dataSocket = accept(listenSocket, NULL, NULL)) == -1) {
            perror("Accept failure");
            exit(EXIT_FAILURE);
        }

    
        char *cmd = malloc(MAX_USER_BUFFER);
        char *filename = malloc(MAX_USER_BUFFER);

        /* get the command and the filename from user's input */
        getCommand(&cmd, &filename);

        /*send the command to the server*/
        sendUserCommand(cmd, filename);

        if (cmd[0] == 'G') {
            //GET command
            char response;
            /*receive response from the server*/
            recv(messageSocket, &response, 1, 0);
            switch(response) {
                case 'G':
                    //save data into the file
                    saveToFile(filename);
                    break;
                case 'B':
                    printf("%s does not exist on the server\n", filename);
                    break;
                default:
                    puts("Server sent back bad response");
                    exit(EXIT_FAILURE);
            }
        } else {//SEND comand
            /*send data in the file to the server*/
            sendFile(messageSocket, dataSocket, filename);
        }
        free(cmd);
        free(filename);

        close(dataSocket);
        close(messageSocket);
    //}
}

/*
 *  FUNCTION: waits on the message socket until there is an incoming request,
 */
void handleMessage(void) {

    /*create the epoll file descriptor.*/
    int epollfd;
    if ((epollfd = epoll_create1(0)) == -1) {
        perror("Epoll create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    ev.data.fd = messageSocket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, messageSocket, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event *eventList = calloc(MAX_EPOLL_EVENTS, sizeof(struct epoll_event));

    int nevents = waitForEpollEvent(epollfd, eventList);
    for (int i = 0; i< nevents; ++i) {
        if (eventList[i].events & EPOLLERR) {
            perror("Socket error");
            close(eventList[i].data.fd);
            continue;
        } else if (eventList[i].events & EPOLLHUP) {
            close(eventList[i].data.fd);
            continue;
        } else if (eventList[i].events & EPOLLIN) {
            //Read from message socket
            char *buffer = calloc(MAX_READ_BUFFER, sizeof(char));
            int n;
            char *bp = buffer;
            size_t maxRead = MAX_READ_BUFFER - 1;
            while ((n = recv(eventList[i].data.fd, bp, maxRead, 0)) > 0) {
                bp += n;
                maxRead -= n;
            }
            buffer[MAX_READ_BUFFER - 1 - maxRead] = '\0';

            if (buffer[0] == 'G') {
                //send file(data) to the client when receiving 'GET' message
                sendFile(messageSocket, dataSocket, buffer + 4);
            } else {
                //Send message
                if (buffer[0] == '\0') {
                    //Client closed the connection on us
                    exit(EXIT_SUCCESS);
                }
                saveToFile(buffer + 5); //receive data and save it to the file when geting 'SEND' message
            }
            free(buffer);
        }
    }
    free(eventList);
    close(epollfd);
}


/*
 *  FUNCTION:  to set to non-blocking mode
 */
void setNonBlocking(int sock) {
    if (fcntl(sock, F_SETFL, O_NONBLOCK | SO_REUSEADDR) == -1) {
        perror("Non blockign set failed");
        exit(EXIT_FAILURE);
    }
}


/*
 *  FUNCTION: return a pointer to a hostent structure of the destination's address
 */
struct hostent * getDestination(void) {
    char *userAddr = getUserInput("Enter the server address: ");
    struct hostent *addr = gethostbyname(userAddr);
    if (addr == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    free(userAddr);
    return addr;
}

/*
 *  FUNCTION: return a string containing the user's input
 *  Input max length is set to 1024. 
 */
char *getUserInput(const char *prompt) {
    char *buffer = calloc(MAX_USER_BUFFER, sizeof(char));
    if (buffer == NULL) {
        perror("Allocation failure");
        abort();
    }
    printf("%s", prompt);
    int c;
    while(true) {
        c = getchar();
        if (c == EOF) {
            break;
        }
        if (!isspace(c)) {
            ungetc(c, stdin);
            break;
        }
    }
    size_t n = 0;
    while(true) {
        c = getchar();
        if (c == EOF || (isspace(c) && c != ' ')) {
            buffer[n] = '\0';
            break;
        }
        buffer[n] = c;
        if (n == MAX_USER_BUFFER - 1) {
            printf("exceed the max user buffer\n");
            memset(buffer, 0, MAX_USER_BUFFER);
            while ((c = getchar()) != '\n' && c != EOF) {}
            n = 0;
            continue;
        }
        ++n;
    }
    return buffer;
}

/*
 *  FUNCTION: send a single char over the commSock corresponding as to whether the desired file was found on the server.
 *  int commSock : The socket used for message communication
 *  int dataSock : The socket used for data transfer
 *  const char *filename - The filename of the file to send
 */
void sendFile(int commSock, int dataSock, const char *filename) {
    FILE *fp = fopen(filename, "r");
    const char bad = 'B';
    const char good = 'G';
    if (fp == NULL) {
        send(commSock, &bad, 1, 0);
        return;
    }
    if (isServer) {
        send(commSock, &good, 1, 0);
    }
    char buffer[MAX_READ_BUFFER];
    memset(&buffer, 0, MAX_READ_BUFFER);
    int nread;
    while ((nread = fread(buffer, sizeof(char), MAX_READ_BUFFER, fp)) > 0) {
        int dataToSend = nread;
        int nSent;
        char *bp = buffer;

start:
        nSent = send(dataSock, bp, dataToSend, 0);
        if (nSent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            goto start;
        }
        if (nSent < dataToSend) {
            dataToSend -= nSent;
            bp += nSent;
            goto start;
        }
    }
    printf("send the file: %s to the client\n", filename);
    fclose(fp);
}


/*
 *  FUNCTION:  get the filename and command.
 *
 *  PARAMETERS:
 *  char **command : A pointer to a string in which to store the command
 *  char **filename : A pointer to a string in which to store the filename
 */
void getCommand(char **command, char **filename) {
    char *message;
    int n;
    while(true) {
        message = getUserInput("Enter GET or SEND followed by the filename: ");
        if ((n = sscanf(message, "%s %s", *command, *filename)) == EOF) {
            perror("sscanf");
            exit(EXIT_FAILURE);
        }
        free(message);
        if (n == 2) {
            if (strncmp(*command, "GET", 3) == 0 || strncmp(*command, "SEND", 4) == 0)  {
                break;
            }
        }
        printf("Invalid Command\n");
    }
}

/*
 *  FUNCTION: waitForEpollEvent

 *  PARAMETERS:
 *  const int epollfd - The epoll desciptor to wait on
 *  struct epoll_event *events - The list of epoll_event structs to fill with the detected events
 *
 *  RETURNS:
 *  int - The number of events that occurred
 */
int waitForEpollEvent(const int epollfd, struct epoll_event *events) {
    int nevents;
    if ((nevents = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1)) == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }
    return nevents;
}

/*
 *  FUNCTION: create a file descirptor, receive data, and put them into the file, max read buffer is 4096
 */
void saveToFile(const char *filename) {
    char *dataBuf = calloc(MAX_READ_BUFFER, sizeof(char));
    int n;
    FILE *fp = fopen(filename, "w");
    while((n = recv(dataSocket, dataBuf, MAX_READ_BUFFER, 0)) > 0) {
        fwrite(dataBuf, sizeof(char), n, fp);
    }
    free(dataBuf);
    fclose(fp);
}

/*
 *  FUNCTION: creating a buffer, filling it with the command and filename
 *  and then sending it through the message socket.
 */
void sendUserCommand(const char *cmd, const char *filename) {
    const size_t BUF_SIZE = strlen(cmd) + strlen(filename) + 1;
    char *buffer = malloc(BUF_SIZE);
    memcpy(buffer, cmd, strlen(cmd));
    buffer[strlen(cmd)] = ' ';
    memcpy(buffer + strlen(cmd) + 1, filename, strlen(filename));

    send(messageSocket, buffer, BUF_SIZE, 0);

    free(buffer);
}
