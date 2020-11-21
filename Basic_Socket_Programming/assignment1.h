/*
 * HEADER FILE: assignment1.h 
 */

#ifndef ASSIGNMENT1_H
#define ASSIGNMENT1_H

#define LISTENPORT 7005
#define MAX_USER_BUFFER 1024
#define MAX_READ_BUFFER 4096
#define MAX_EPOLL_EVENTS 100

/*function declaration*/
void runServer(void);
void runClient(void);
void handleMessage(void);
void setNonBlocking(int sock);
struct hostent * getDestination(void);
char *getUserInput(const char *prompt);
void sendFile(int commSock, int dataSock, const char *filename);
void getCommand(char **command, char **filename);
int waitForEpollEvent(const int epollfd, struct epoll_event *events);
void saveToFile(const char *filename);
void sendUserCommand(const char *cmd, const char *filename);

#endif