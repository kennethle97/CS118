# CS118 Project 1

This is the repo for spring23 cs118 project 0.

## Academic Integrity Note

You are encouraged to host your code in private repositories on [GitHub](https://github.com/), [GitLab](https://gitlab.com), or other places.  At the same time, you are PROHIBITED to make your code for the class project public during the class or any time after the class.  If you do so, you will be violating academic honestly policy that you have signed, as well as the student code of conduct and be subject to serious sanctions.

## Provided Files

- `project` is folder to develop codes for future projects.
- `docker-compose.yaml` and `Dockerfile` are files configuring the containers.

## Bash commands

```bash
# Setup the container(s) (make setup)
docker compose up -d

# Bash into the container (make shell)
docker compose exec node1 bash

# Remove container(s) and the Docker image (make clean)
docker compose down -v --rmi all --remove-orphans
```

## Environment

- OS: ubuntu 22.04
- IP: 192.168.10.225. NOT accessible from the host machine.
- Port forwarding: container 8080 <-> host 8080
  - Use http://localhost:8080 to access the HTTP server in the container.
- Files in this repo are in the `/project` folder. That means, `server.c` is `/project/project/server.c` in the container.

## TODO

Kenneth Le #804953883

High Level Design:

The general functionality of my server program is as follows:

1.) Set up the sockets for the server and binding them to port numbers. For this project I ran the server locally at IP Address 127.0.0.1 and binded specifically to port 8080.

2.)Listen in on the socket for incoming connections from potential clients.By polling for them.

3.)Once a client attempts to connect we accept the connection and create a socket for the client to communicate.

4.)Wait for a request from the client using recv and then parse the request for the given command and filename.

5.)If the file doesn't exist we return a 404 error and if it does we send a HTTP Response header that includes the Content-Type/MIME TYPE and the Content Length. Followed by the contents of the file requested.

6.)Once the contents have been transmitted.We close the file descriptor for that file and either wait for a new request from the client or close the connection.

Problems Encountered:

In the beginning setting up the server was not too difficult as most of the skeleton code was provided either through the links supplied in the project description or through the discussion slides. The more tedious part was initially getting the file contents. Initially I had a problem where I was getting the correct header response but the filecontents would not be outputted correctly through the browser specifically when trying to get the JPG files. I realized I had made a very stupid error as I was trying to send both at the same time through one call of send().By sending the response header and the filecontents seperately my jpg and jpeg files were able to load correctly.The next big issue was figuring out how to do case insensitivity for filenames. My approach was to get the filename from the request and convert it all to lowercase and then parse my files in my directory and convert them to lowercase as well to check for a match. As such I had to learn about dirent.h methods to be able to implement the ability to parse through the current directory where the executable was located.Another issue I had was with being able to parse % correctly and spaces. Specifically in the case where I was texting a URL with a space it would always convert the spaces in the url to %20. Initially I was trying to decode the URL first for spaces and then check for case insensitivity. When I did this however I would not be able to resolve the cases where the filename explicitly included %20 as shown in my example with Sample%20text.txt as it would be converted to Sample text.txt and then would return a NULL pointer after checking. To fix this I just passed it through the case-insensitivity function to see if the file did exist and then if it did return that filename and if it didn't pass it through the decoder and the case-insensivity function to look again.

Resources I used: https://beej.us/guide/bgnet/html/split/ For learning the berkeley sockets in the C/C++ I used this resource as well as the discussion slides to establish the basic functionality of establishing the server as well as understanding send() and recv() for transfering data between server/client.

For learning how to parse through the directory:

https://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html
https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
https://stackoverflow.com/questions/25070751/navigating-through-files-using-dirent-h

For learning how to handle spaces.
https://stackoverflow.com/questions/5442658/spaces-in-urls



    ###########################################################
    ##                                                       ##
    ## REPLACE CONTENT OF THIS FILE WITH YOUR PROJECT REPORT ##
    ##                                                       ##
    ###########################################################