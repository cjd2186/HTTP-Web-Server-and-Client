This code implementsa program that is a limited version of wget that can download a single file
  to the current directory given a host machine, port number and file path.

The http-client program works as follows:
    The file from the given file path is read from its host machine, and the 
    contents of the file is written to a new file in the current directory using
    the same file name.

The http-client.c file was implemented as follows:
    First the host name inputted in the commandline argument is converted
    into an IP address.
    Next, a socket for the TCP connection is created and connected to IP
    address. 
    A GET request in then created with the file path, host machine, and port
    number.
    This GET request is sent to the host server, and the client then reads
    the server's response as a file.
    If the server response does not start with "200 OK", the client program
      terminates and shows an error message.
    Upon reading the response, the headers following the first 200 OK line
    are bypassed until a blank line is read, preceding the message body.
    The file body is read from and written to a new file one byte at a time,
    to ensure that the entire file body is downloaded properly.
    Finally, the proper resources are closed and the client program
    terminates.
