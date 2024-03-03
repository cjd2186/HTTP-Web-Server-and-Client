#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

/*
 * http-client.c
 * This program is a limited version of wget that can download a single file
 *   to the current directory given a host machine, port number and file path.
 * The file from the given file path is read from its host machine, and the contents
 *   of the file is written to a new file in the current directory using the same
 *   file name.
 * @author: Christopher Demirjian
 * uni: cjd2186
 *
 */

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv){
        
    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port number> <file path>\n", argv[0]);
        exit(1);
    }

    struct hostent *he;
    char *serverName = argv[1];

    // get server ip from server name
    if ((he = gethostbyname(serverName)) == NULL) {
        die("gethostbyname failed");
    }
    char *serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);
    
    unsigned short port = atoi(argv[2]);

    // Create a socket for TCP connection

    int sock; // socket descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    // Construct a server address structure

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); // must zero out the structure
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serverIP);
    servaddr.sin_port        = htons(port); // must be in network byte order

    // Establish a TCP connection to the server

    if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("connect failed");

    //create the GET request and store into buffer
    char getreq [1000];
    sprintf(getreq, "GET %s HTTP/1.0\r\nHost: %s:%d\r\n\r\n", argv[3], serverName, port);

    size_t len= strlen(getreq);

    //send getreq to the server
    if (send(sock, getreq, len, 0) != len)
        die("send failed");
    //read in the response from the socket
    FILE *serv_in= fdopen(sock, "r");

    char line [500];
    //store the filename from the file path
    char *name= strrchr(argv[3],'/');
    name[strlen(name)]='\0';
    char *file_name= name +1;
    FILE *serv_out=fopen(file_name, "w");

    //response 

    //read in the first line of the response
    fgets(line, sizeof(line), serv_in);
    //if response is not 200 OK, an error occurred and body cannot be read
    if(!(strstr(line, "200 OK"))){
        fclose(serv_in);
        die("404 Not Found");
    }

    else{
        //parse the response until a blank line in found
        while(strcmp(line, "\r\n")!=0){
            fgets(line, sizeof(line), serv_in);
        }
        //write the response to a new file in the directory
        //read and write each byte 1 by 1 to ensure that no bytes are missing in the outputted file
        while((fread(line, 1, 1, serv_in))!=0){
            fwrite(line, 1, 1, serv_out);
        }
        
    }
    //close resources
    fclose(serv_in);
    fclose(serv_out);
    return 0;
}
