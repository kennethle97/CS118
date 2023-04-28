#include <iostream>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>


#define MY_PORT 8080
#define BACKLOG 100 /*Pending connections queue size*/
#define BUFFER_SIZE 1024


int main(int argc, char*argv[])
{
    int sock_fd , client_fd; /*socket file descriptors for server and client*/
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int port_number = MY_PORT;

    if((sock_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(server_addr.sin_zero, '\0',sizeof(server_addr.sin_zero));

    /*Binding the socket*/
    printf("Binding socket to %d\n",port_number);

    if(bind(sock_fd, (struct sockaddr*) &server_addr, 
        sizeof(server_addr)) == -1){
            perror("Error in binding the socket");
            exit(1);
        }

    int yes=1;

    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1){
        perror("Cannot reset socket options on sock_fd");
        exit(1);
    }

    if(listen(sock_fd, BACKLOG) == -1){
        perror("Error in listening to socket");
        exit(1);
        }

     printf("Listening to socket %d\n",port_number);

    while(1){

        printf("Waiting to accept connection from client.\n");

        client_fd = accept(sock_fd, (struct sockaddr *) &client_addr,&client_addr_size); 
        if(client_fd == -1){
            perror("Error in accepting client connection");
            exit(1);
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        printf("Accepted connection from client:\n %s:%d\n", client_ip, client_port);

    }
}
