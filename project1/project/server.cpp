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
    /*Navigate through the directory given by dir_path to look for the file*/
    if((directory = opendir(dir_path)) != NULL){
        while((dir = readdir(directory)) != NULL){
            //checking to see if it's a normal file with DT_REG flag
            if(dir -> d_type == DT_REG){
                //Initialize temporary pointer to character array holding filename we are iterating over.
                char *temp_filename = dir -> d_name;
                int temp_length = strlen(temp_filename);
                char lower_temp_filename[temp_length+1];
                //Change the filename to lowercase
                for(int i = 0; i < temp_length; i++){
                    lower_temp_filename[i] = tolower(temp_filename[i]);
                }
                //check and see if we get a match and if so return a copy of the string to a buffer. 
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
        //Shouldn't usually return anything as the directory is already configured in main was used for debugging.
        printf("Directory path not found on system:%s\n",dir_path);
    }
    return NULL;

}

char* decode_filename(const char* filename){
    /*Primary goal of this function is to be able to read character spaces in file names and return it*/
    int filename_length = strlen(filename);
    char* filename_decoded = (char*)malloc(filename_length+1);
    int j = 0;

    /*Parse through the filename to look for %20 symbol representing a space and returning the string with a space*/
    for(int i = 0; filename[i] != '\0';i++){
        if(filename[i] == '%'){
            if(filename[i+1] == '2' && filename [i+2] == '0'){
                filename_decoded[j] = ' ';
                i+=2;
            }
            else{
            filename_decoded[j]=(char)strtol(&filename[i+1],NULL, 16);
            i+=2;
            }
        }
        else{
            filename_decoded[j] = filename[i];
        }
        j++;
    }

    return filename_decoded;

}
/*mime_type returns the mime type of the filename passed into it*/
const char *mime_type(char* filename){
    /*By default mime_type is set to application/octet-stream as a generic file type*/
    const char *mime_type = "application/octet-stream";
    if (strstr(filename,".txt")){
        mime_type = "text/plain";
    }
    else if(strstr(filename, ".html") || strstr(filename, ".htm")){
        mime_type = "text/html";
    }
    else if (strstr(filename,".jpg") || strstr (filename,".jpeg")){
        mime_type = "image/jpeg";
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

    /*Set current working directory to wherever the executable is run from including the files.*/
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
        char* filename_buffer = new char[256];
        memset(filename_buffer,0,256);
        sscanf(buffer, "GET /%256[^? ]? HTTP/1.1",filename_buffer);
        printf("Filename_Buffer:%s\n",filename_buffer);
        char* filename;
        /*First we pass the filename from the request and pass it directly into find_case_insensitive_filename to see if we get a match.
         If we don't then we pass it into the decoder and then run it through case_insensitive test to see if we get a match again.*/
        if(find_case_insensitive_filename(filename_buffer,directory_path)!= NULL){
            filename = find_case_insensitive_filename(filename_buffer,directory_path);
        }
        else{
            char *decoded_filename = decode_filename(filename_buffer);
            printf("Decoded_Filename:%s\n",decoded_filename);
            filename = find_case_insensitive_filename(decoded_filename,directory_path);
            printf("Filename:%s\n",filename);
        }
        //Now to open file and read read contents into buffer
        struct stat file_stat;
        char* file_content;
        int file_size;

        int fd = open(filename,O_RDONLY);
        /*Return 404 Response if filename doesn't exist */
        if (fd == -1){
            char response[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client_fd,response,strlen(response),0);
            }
        else{
            /*First we send a response header with mime_type and file_size and then send the contents of the file afer.*/
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
        /*close the file descriptor after finishing sending the data*/
        close(fd);
        }
        /*Close the remaining sockets and the server.*/
    close(client_fd);
    close(sock_fd);
}