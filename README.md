# chatServer
Chat Server Implementation using Select and Non-blocking Sockets
Authored by Guy Cohen
206463606 

# Description
This exercise requires the implementation of an event-driven chat server where incoming messages are broadcasted to all connected clients except for the one that sent the message. 
The challenge of this exercise is to implement this behavior in an entirely event-driven manner, without the use of threads.


# Program DATABASE:
The    code defines two structs, conn_pool_t and conn_t, which are used to keep track of the connections and the messages in the chat server. 
conn_pool_t is used to store information about the connection pool, including the maximum file descriptor, the number of ready descriptors and sets of all active descriptors for reading and writing. 
conn_t is used to keep track of the state of individual client connections and includes file descriptor, pointers to the head and tail of the doubly-linked list of messages that need to be written out on this connection.
msg_t is used to keep track of messages, it includes pointers to the previous and next messages in the doubly-linked list, the message buffer, and the size of the message. The doubly-linked list structure allows for efficient insertion and removal of connections and messages as clients connect and disconnect from the server, and as messages are sent and received.

# Program flow:
The program will perform the following actions:

1. Parse the command line arguments.
2. Connect to the server.
3. Construct an HTTP request based on the options specified in the command line.
4. Send the HTTP request to the server.
5. Receive an HTTP response.
6. Display the response on the screen.

This program will implement this two files:
 threadpool.c: This file contains the implementation of the methods from the header file 'threadpool.h'. So that different actions can be made on the threadpool, such as initialization, dispatch, dowork, destroy and various actions on the job queue
server.c: The file 'server.c' contains the main function which receives an array of arguments. The first argument is the port number, followed by the number of threads and the number of requests that the server can handle. After checking the input, the server opens a socket and uses the threadpool to handle HTTP requests from clients. Depending on the type of request, such as an error or a request for a folder or file, the server returns a reply to each client through the socket.

# functions:
The main function of the chat server program sets up a signal handler for the SIGINT signal, which is used to handle the interruption of the program by the user. It initializes a conn_pool_t structure, which is used to keep track of all active client connections.
It creates a socket, sets it to be non-blocking, binds it to a port, and sets the listen backlog. It then enters a loop where it uses the select function to monitor socket descriptor readiness, and handle incoming connections, incoming data and data to be written to clients. The program continues to run in this loop until it is interrupted by the user or an error occurs. When the program exits, it releases any allocated resources and cleans up memory.
The init_pool function is called to initialize the conn_pool_t structure, which is used to keep track of all active client connections.

The add_conn function is called when a new client connects to the server. This function adds the new connection to the conn_pool_t structure and updates the relevant data structures.

The remove_conn function is called when a client closes their connection or when the server is stopped. This function removes the connection from the conn_pool_t structure and releases any associated resources.

The add_msg function is called when a message is received from a client. This function adds the message to the queue of all connections (except for the origin) in the conn_pool_t structure.

The write_msg function is called to write a message to a client. It takes the socket descriptor of the connection to write the message to and the conn_pool_t structure as input.

**The program uses the select function to monitor socket descriptor readiness and handle incoming connections and data transfer in an event-driven manner, without the use of threads.

**The conn_pool_t, conn_t, and msg_t structs are used to keep track of the state of connected clients, their file descriptors, and the messages they send and receive.

# Output:
The server will listen at a specified port number for incoming connections, which is passed as a parameter to the program. When a new connection request arrives from some client, the server accepts the connection and creates a new struct conn connection object for that client. The server reads a message from one client and then adds that message to all other connected clients by creating a "msg" object and adding it to their sockets. The server will only read another line of input from a client once the previous line has been processed and will only add a socket to the write set if there is something to write to it. The implementation details can be found in the chatServer.h and chatServer.c files.
# Program Files:
chatSrever.c chatServer.h

# How to compile?
compile:   gcc -g -Wall chatServer.c -o server 
run: ./server <port>

****(To connect to the chat server using Telnet, open a Command Prompt or Terminal window and type "telnet [server IP or hostname] [port number]" (without the quotes). Replace "[server IP or hostname]" with the IP address or hostname of the server where the chat server is running, and replace "[port number]" with the port number on which the chat server is listening. This will establish a connection to the chat server and allow you to participate in the chat.)
