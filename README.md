# CPE400Project
Implement a file transfer application
Write a simple client-server socket programming. This program will be used to transfer files from client to server. The server will be listening on a port (say port #5050) and the Client will connect to Server and transfer files to Server. Here are the details:

1. Client will take the source folder path (i.e., folder that contains files) and the number of concurrent file transfers as command-line arguments. For example, "java Client folder 5" will transfer files in “myFolder” folder to destination five at a time. In other words, concurrent file transfer means transferring multiple files over separate connections to increase overall throughput. If the concurrency number is not entered, it should transfer one file at a time (aka concurrency=1), by default.

2. The application should support integrity verification. That is, your client and server will calculate the checksum of each file after it is transferred and compare them to make sure data is transferred without any error in the network.

# Compile instructions
$ g++ server.cpp -o server -lws2_32
$ ./server

$ g++ client.cpp -o client -lws2_32 -lmswsock
$ ./client C:\\...\\<source-folder>\\ <concurrency>

