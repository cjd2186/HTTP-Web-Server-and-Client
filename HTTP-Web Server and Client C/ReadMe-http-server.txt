Http-server.c Implementation:

Every error status executes properly, both the static and dynamic webpages
can be served as well.

The http-server must be terminated manually using ctrl-C, and will serve
clients in succession, but not simultaneously.
The http-server is able to take GET requests from netcat/firefox,
  and deliver the necessary files to the client.
The http-server is also able to send a dynamic webpage to a browser,
  in which mdb-lookup can be executed, accessing the mdb-lookup-server.
