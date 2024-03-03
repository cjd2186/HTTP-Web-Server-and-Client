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

#define BUF_SIZE 4096
/*
 * http-server.c
 * This program is used as an intermediate server between a mdb-look-server and firefox browser.
 * @author: Christopher Demirjian
 * uni: cjd2186
 *
 */

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv){
        
    if (argc != 5) {
        fprintf(stderr, "usage: %s <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }

    /////////////////////
    //**Client Socket**//
    /////////////////////


    //make connection to mdb-lookup-server
    char *serverName;
    char *serverIP;
    char *serverPort;
    int sock;
    struct sockaddr_in serverAddr;
    struct hostent *he;

    // parse args
    serverName = argv[3];
    serverPort = argv[4];

    // get server ip from server name
    if ((he = gethostbyname(serverName)) == NULL) {
	die("gethostbyname failed");
    }
    serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);

    // create socket
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	die("socket failed");
    }

    // construct server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    unsigned short port = atoi(serverPort);
    serverAddr.sin_port = htons(port);

    // connect
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
	die("connect failed");
    }


    //*//////////////*//
    //*Server  Socket*//
    //*//////////////*//

    //Take in the arguments and any information that the server will need to use
    // Server port, web root
    port = atoi(argv[1]);
    char *rootdir= argv[2];
    // Create a listening socket (also called server socket)

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    // Construct local address structure

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr; 

    while (1) {
        //accepting an incoming connection
        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,
                        (struct sockaddr *) &clntaddr, &clntlen)) < 0)
            die("accept failed");

        // accept() returned a connected socket (also called client socket)
        // and filled in the client's address into clntaddr
        FILE * clnt_in= fdopen(clntsock, "r");
        if (clnt_in == NULL)
            die("fdopen failed");
        
        char requestLine[1000];

        //the GET request is the first line of the request, therefore read the first line
        char *statusCode= "200 OK";
        int noCtrlC=1;
        //if client interrupts connection/GET request with Ctrl-C
        if(fgets(requestLine, sizeof(requestLine), clnt_in)==NULL){
            fprintf(stderr, "%s \"  \" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr));
            noCtrlC=0;
        }
        //if client doesnt end mid-request with Ctrl C
        if(noCtrlC){
            //store each component of the GET request
	
	    char *token_separators = "\t \r\n"; // tab, space, new line
            char *method = strtok(requestLine, token_separators);
            char *requestURI = strtok(NULL, token_separators);
            char *httpVersion = strtok(NULL, token_separators);
	
	    //go through each scenario for error codes
	    //to prevent conditional jumps, check each part isnt null first
            //if request URI is simply "/mdb-lookup", send submit form


            int notmdb=1;
            char mresponse[1000];
            //check for URI input
            if(requestURI){
                //block if no key entered
                if(strcmp(requestURI, "/mdb-lookup")==0){
                    // send 200 OK and form to http server
                    const char *form =
                        "<h1>mdb-lookup</h1>\n"
	                "<p>\n"
        	        "<form method=GET action=/mdb-lookup>\n"
        	        "lookup: <input type=text name=key>\n"
	                "<input type=submit>\n"
        	        "</form>\n"
        	        "<p>\n";
                    sprintf(mresponse, "HTTP/1.1 200 OK\r\n\r\n%s\r\n",form);
                    size_t len=strlen(mresponse);
	            //send response
                    if (send(clntsock, mresponse, len, 0) != len) {
                        die("send failed");
                    }     
                    //used to block off dynamic content from static content
                    notmdb=0;        
                }
            }

            if(requestURI){
                if(strncmp(requestURI, "/mdb-lookup?key=",16)==0){
                    //read rest of GET request, skip header lines
                    //httpVersion= "HTTP/1.1";
                    char buf[BUF_SIZE];
	            for (;;) {
	                if (fgets(buf, sizeof(buf), clnt_in) == NULL) {
	                    if (ferror(clnt_in))
	                        die("IO error");
	                    else {
	                       fprintf(stderr, "server terminated"
	                       " connection without sending file");
	                       fclose(clnt_in);
	                       exit(1);
	                    }
	                }
	                if (strcmp("\r\n", buf) == 0) {
	                    // this marks the end of header lines
	                    // get out of the for loop.
	                    break;
	                }
                    }
                    char *search= strchr(requestURI, '=');
                    //key is the search term
                    char *key= search +1;
                    //skey is same as key, but with a newline character
                    char skey[1000];
                    strcpy(skey, key);
                    strcat(skey, "\n");
                    if (send(sock, skey, strlen(skey), 0) != strlen(skey)) {
                        die("send failed");
                    }
                    sprintf(requestURI, "/mdb-lookup?key=%s", key);
                    //now can read from socket to get back lookup results
                    FILE *mdb_in= fdopen(sock, "r");
                    if (mdb_in == NULL)
                        die("fdopen failed");
                    int color=1;
                    char table[1000];
                    //must send the form before sending the table
                    const char *form =
                        "<h1>mdb-lookup</h1>\n"
	                "<p>\n"
        	        "<form method=GET action=/mdb-lookup>\n"
        	        "lookup: <input type=text name=key>\n"
	                "<input type=submit>\n"
        	        "</form>\n"
        	        "<p>\n";
                    sprintf(mresponse, "HTTP/1.1 200 OK\r\n\r\n%s\r\n",form);
                    size_t len=strlen(mresponse);
                    
                    fprintf(stderr, "looking up [%s]: ", key);
	            //send response
                    if (send(clntsock, mresponse, len, 0) != len) {
                        die("send failed");
                    }
                    //sen mdb-lookup results in table, piece by piece, making even rows yellow
                    sprintf(table, "<p><table border>\n");
                    if (send(clntsock, table, strlen(table), 0) != strlen(table)){
                        die("send failed");
                    }
                    while ((fgets(buf, sizeof(buf), mdb_in)) != NULL) {
	                //send file as you read it
                        char row[1000];
                        if(color%2==0){
                            sprintf(row, "<tr><td bgcolor=yellow>");
                        } 
                        else{
                            sprintf(row, "<tr><td>");
                        }
                        strcat(row, buf);
                        strcat(row, "\n");
                        if (send(clntsock, row, strlen(row), 0) != strlen(row)) {
                            die("send failed");
                        }
                        color++;
                        //new line means end of results: so break out of fgets loop
                        if (strcmp(buf, "\n")==0){
                            break;
                        }
                    }
                    //send footer for table
                    char lastrows[1000];
                    sprintf(lastrows, "</table>\n</body></html>\n");
                    if (send(clntsock, lastrows, strlen(lastrows), 0) != strlen(lastrows)) {
                        die("send failed");
                    }
                    if (ferror(mdb_in)) {
                        perror("fgets failed to read from input");
                    }
                    notmdb=0;
                }
            }
            //used to prioritize 400 Bad Request over other error codes that may override it
            int nofourHun=1;
	    if(requestURI){
	        //URI must start with "/"
	        if(strncmp (requestURI, "/", 1) !=0){
	            statusCode= "400 Bad Request";
                    nofourHun=0;
	        }
                //cannot have .. its unsecure!
	        if(strstr(requestURI, "..")){
	            statusCode= "400 Bad Request";
                    nofourHun=0;
	        }
                //make sure this is not for the dynamic mdb-lookup, as there are different
                //  errors to be had for each service
                if(strncmp(requestURI, "/mdb-lookup", 11)!=0){
                    if(nofourHun){
                    //check file exists
                    char path [1000];
                    sprintf(path, "%s%s", rootdir, requestURI);
                    FILE *serv_out=fopen(path, "r");
                    if(serv_out==NULL){
                        statusCode= "404 Not Found";
                    }   
                    //check if its a directory without "/" at end
                    struct stat stbuf;                 
                    if(stat(path, &stbuf) == -1){
                        statusCode= "404 Not Found";
                    }
                    //checks if file is a directory
                    if((stbuf.st_mode & S_IFMT) == S_IFDIR){
                        char *name= strrchr(requestURI,'/');
                        name[strlen(name)]='\0';
                        char *file_name= name +1;
                        //length will not be 0 if the last instance
                        //of "/" is not at end of requestURI
                        if ((strlen(file_name))!=0){
                           statusCode= "403 Forbidden";
                        }
                    }
                    }  
                }
	    }
	    if(method){
	        //only process GET methods 
	        //method must be GET
	        if(!(strstr(method, "GET"))){
	            statusCode= "501 Not Implemented";
	        }
            }
	    if(httpVersion){
	            //http must be 1.0 OR 1.1
	            if(!((strstr(httpVersion, "HTTP/1.0") || strstr(httpVersion, "HTTP/1.1")))){
	                statusCode= "501 Not Implemented";
	            }
	    }
            //501 if a parameter is missing from the GET request
            if ((requestURI==NULL)||(method==NULL)||httpVersion==NULL){
	            statusCode="501 Not Implemented";
	    }
            //print status to server std error
	    fprintf(stderr, "%s \"%s %s %s\" %s \n", inet_ntoa(clntaddr.sin_addr),  method, requestURI, httpVersion, statusCode);
            char response[1000];
            if ((strcmp(statusCode, "200 OK"))!=0){
                //error occurred, send status code to client, send HTML to client
                sprintf(response, 
                    "HTTP/1.0 %s\r\n"
                    "\r\n"
                    "<html><body>\r\n"
                    "<h1>%s</h1>\r\n"
                    "</body></html>\r\n", statusCode, statusCode);
                size_t len=strlen(response);
                //send response
                if (send(clntsock, response, len, 0) != len)
                    die("send failed");

            }
            else{
               //no error occured, proceed
               //this part is only for the static page, so must not be serving mdb-lookup
              if(notmdb){
                //create respones to send to client confirming 200 OK
	        sprintf(response, "\r\nHTTP/1.0 %s\r\n\r\n", statusCode);
	            
	        size_t len=strlen(response);
	        //send response
	        if (send(clntsock, response, len, 0) != len)
	            die("send failed");
                
	        //append "index.html" to the URI if it ends in "/"
	        char *name= strrchr(requestURI,'/');
	        name[strlen(name)]='\0';
	        char *file_name= name +1;
	        //length will not be 0 if the last instance
	        //of "/" is not at end of requestURI
	        if ((strlen(file_name))==0){
	            strcat(requestURI, "index.html");
	        }
	            
	        //read rest of GET request, skip header lines
	        char buf[BUF_SIZE];
	        for (;;) {
	            if (fgets(buf, sizeof(buf), clnt_in) == NULL) {
	                if (ferror(clnt_in))
	                    die("IO error");
	                else {
	                    fprintf(stderr, "server terminated"
	                     " connection without sending file");
	                    fclose(clnt_in);
	                    exit(1);
	                }
	            }
	            if (strcmp("\r\n", buf) == 0) {
	                // this marks the end of header lines
	                // get out of the for loop.
	                break;
	            }
	        }
	
	        //header has been parsed: now file is to be read
	        char path [1000];
	        sprintf(path, "%s%s",rootdir, requestURI);
	        FILE *serv_out=fopen(path, "r");
	        //read from file and send to client
	        size_t n;
	        while ((n = fread(buf, 1, sizeof(buf), serv_out)) > 0) {
	            //send file as you read it
	            if (send(clntsock, buf, n, 0) != n)
	                die("send failed");
	            // fread() returns 0 on EOF or on error
	            // so we need to check if there was an error.
	            if (ferror(serv_out))
	                die("fread failed");
	        }
            //close sockets
            fclose(serv_out);
            }
          }
        }
        fclose(clnt_in); 
    }
}
