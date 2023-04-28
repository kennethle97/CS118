#include <iostream>
#include <cstring>
#include <cstdio>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>


#define MY_PORT 8080
#define BACKLOG 10 /*Pending connections queue size*/
#define BUFFER_SIZE 1024
#define MAX_PATH_LENGTH 1024

char* find_case_insensitive_filename(const char* filename ,const char* dir_path){
    DIR *directory;
    struct dirent *dir;
    int filename_length = strlen(filename);
    char lower_filename[filename_length + 1];
    //Convert filename to lowercase for parsing through directory later.
    for(int i = 0; i < filename_length;i++){
        lower_filename[i] = tolower(filename[i]);
    } 

    if((directory = opendir(dir_path)) != NULL){
        while((dir = readdir(directory)) != NULL){
            //checking to see if it's a normal file
            if(dir -> d_type == DT_REG){
                char *temp_filename = dir -> d_name;
                int temp_length = strlen(temp_filename);
                char lower_temp_filename[temp_length+1];
                for(int i = 0; i < temp_length; i++){
                    lower_temp_filename[i] = tolower(temp_filename[i]);
                } 
                bool file_match = (strcmp(lower_temp_filename, lower_filename));
                if(file_match == 0){
                    char* filename_match = (char*)malloc(strlen(temp_filename)+1);
                    strcpy(filename_match, temp_filename);
                    return filename_match;
                }
            }
        }
    }
    else{
        printf("Directory path not found on system:%s\n",dir_path);
    }
    return NULL;

}


const char *mime_type(char* filename){
    const char *mime_type = "application/octet-stream";
    if (strstr(filename,".txt")){
        mime_type = "text/plain";
    }
    else if(strstr(filename, ".html") || strstr(filename, ".htm")){
        mime_type = "text/html";
    }
    else if (strstr(filename,".jpg") || strstr (filename,"jpeg")){
        mime_type = "image/jpg";
    }
    else if (strstr(filename,".png")){
        mime_type = "image/png";
    }
    return mime_type;
}


int main(int argc, char*argv[])
{
    int sock_fd , client_fd; /*socket file descriptors for server and client*/
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int port_number = MY_PORT;


    char directory_path[MAX_PATH_LENGTH];
    if(getcwd(directory_path,MAX_PATH_LENGTH) == NULL){
        perror("Error in getting current working directory");
        exit(1);
    }

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

    /*Fixes error when port number is in use.*/
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

     printf("Waiting to accept connection from client.\n");

    while(1){
        //Accepting client connections to server 
        client_fd = accept(sock_fd, (struct sockaddr *) &client_addr,&client_addr_size); 
        if(client_fd == -1){
            perror("Error in accepting client connection");
            exit(1);
        }
        //getting client_ip address and port number and printing it to console.
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        printf("Accepted connection from client:\n %s:%d\n", client_ip, client_port);


        //Now to read HTTP request from client

        memset(buffer, 0, BUFFER_SIZE);
        recv(client_fd, buffer, BUFFER_SIZE, 0);
        printf("Request Received:\n%s\n",buffer);

        //Parse HTTP request for filename
        char* filename = new char[256];
        memset(filename,0,256);
        sscanf(buffer, "GET /%s HTTP/1.1",filename);

        filename = find_case_insensitive_filename(filename,directory_path);

        //Now to open file and read read contents into buffer
        struct stat file_stat;
        char* file_content;
        int file_size;

        int fd = open(filename,O_RDONLY);
        if (fd == -1){
            char response[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client_fd,response,strlen(response),0);
            }
        else{
            fstat(fd, &file_stat);
            file_size = file_stat.st_size;
            file_content = (char*)malloc(file_size+1);
            memset(file_content,0,file_size);
            read(fd, file_content, file_size);
            const char* mime_typestr = mime_type(filename);
            char response[BUFFER_SIZE] = {0};
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", mime_typestr, file_size);
            send(client_fd, response, strlen(response), 0);
            send(client_fd, file_content, file_size, 0);
            
            free(file_content);
        }

        close(fd);
        }
    close(client_fd);
    close(sock_fd);
}