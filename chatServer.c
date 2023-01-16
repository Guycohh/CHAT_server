#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <ctype.h>
#include "chatServer.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
//--------------------------functions--------------------------------------
bool isnumber (char* str);//check is string is a number.
void remove_messages(msg_t *m);// remove a message from the list.
void delete_memory(conn_pool_t* pool);// free allocated memory.
void max_fd_array(conn_pool_t* pool);// array[1000] contains the open fds, so this function return th max fd.
//--------------------------functions--------------------------------------
//--------------------------defines/variables------------------------------
#define USAGE "Usage: server <port>"
static int end_server = 0;
int array[1000];// contains the open fds.
//--------------------------defines/variables------------------------------

/* use a flag to end_server to break the main loop */
void intHandler(int SIG_INT) {
    end_server=1;
}

int main (int argc, char *argv[])
{
    signal(SIGINT, intHandler);

    if(argc!=2 || !isnumber(argv[1])){
        printf(USAGE);
        return 0;
    }
    int port= atoi(argv[1]);
    if(port<1 || port>65536){
        printf(USAGE);
        return 0;
    }
//    Data structure to keep track of active client connections (not the for main socket).
	conn_pool_t* pool = malloc(sizeof(conn_pool_t));
    if(pool==NULL) {
        printf(USAGE);
        exit(-1);
    }
	init_pool(pool);
	/*************************************************************/
	/* Create an AF_INET stream socket to receive incoming      */
	/* connections on                                            */
	/*************************************************************/

    int main_socket;  //create an endpoint for communication
    if((main_socket= socket( AF_INET, SOCK_STREAM , 0) )< 0){ //checking success
        printf(USAGE);
        delete_memory(pool);
        exit(-1);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port= htons(port);

	/*************************************************************/
	/* Set socket to be nonblocking. All of the sockets for      */
	/* the incoming connections will also be nonblocking since   */
	/* they will inherit that state from the listening socket.   */
	/*************************************************************/
    //	ioctl(...);
    int on = 1;
    int rc;
    if((rc=ioctl(main_socket, (int)FIONBIO, (char *)&on))!=0){
        close(main_socket);
        printf(USAGE);
        exit(-1);
    }
    /*************************************************************/
	/* Bind the socket                                           */
	/*************************************************************/
    //	bind(...);
    if(bind(main_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))<0){//bind the socket to the port we want.
        close(main_socket);
        delete_memory(pool);
        printf(USAGE);
        exit(-1);
    }

	/*************************************************************/
	/* Set the listen back log                                   */
	/*************************************************************/
//	listen(...);
    if(listen(main_socket,5)<0){//good number for a queue is 5...
        close(main_socket);
        delete_memory(pool);
        printf(USAGE);
        exit(-1);
    }

	/*************************************************************/
	/* Initialize fd_sets  			                             */
	/*************************************************************/
    FD_SET(main_socket, &(pool->read_set));
    pool->maxfd = main_socket;
    /*************************************************************/
	/* Loop waiting for incoming connects, for incoming data or  */
	/* to write data, on any of the connected sockets.           */
	/*************************************************************/
    int sd;
    char buffer[BUFFER_SIZE];
    memset(array, 0, 999);
    array[0]=array[1]=array[2]=-1;
	do
	{
        /**********************************************************/
		/* Copy the master fd_set over to the working fd_set.     */
		/**********************************************************/
        pool->ready_write_set=pool->write_set;
        pool->ready_read_set=pool->read_set;
		/**********************************************************/
		/* Call select() 										  */
		/**********************************************************/
//        max_fd_array(pool);

        // pool->nready is the number of ready descriptors returned by select.
        printf("Waiting on select()...\nMaxFd %d\n", pool->maxfd);

        if(((pool->nready=select(pool->maxfd+1, &(pool->ready_read_set),&(pool->ready_write_set),0, 0))<0)){
            continue;
        }
		/**********************************************************/
		/* One or more descriptors are readable or writable.      */
		/* Need to determine which ones they are.                 */
		/**********************************************************/

		for(int i=3; i<=(pool->maxfd); i++) {
            /* Each time a ready descriptor is found, one less has  */
            /* to be looked for.  This is being done so that we     */
            /* can stop looking at the working set once we have     */
            /* found all of the descriptors that were ready         */
            /*******************************************************/
            /* Check to see if this descriptor is ready for read   */
            /*******************************************************/
            if (FD_ISSET(i, &(pool->ready_read_set))) {
                if (i == main_socket) {
                    /***************************************************/
                    /* A descriptor was found that was readable		   */
                    /* if this is the listening socket, accept one      */
                    /* incoming connection that is queued up on the     */
                    /*  listening socket before we loop back and call   */
                    /* select again. 						            */
                    /****************************************************/
                    //create socket for the client.
                    if ((sd = accept(main_socket, NULL, NULL)) < 0) {
                        delete_memory(pool);
                        exit(-1);
                    }
                    printf("New incoming connection on sd %d\n", sd);
                    add_conn(sd, pool);
                }
                else {
                    /****************************************************/
                    /* If this is not the listening socket, an 			*/
                    /* existing connection must be readable				*/
                    /* Receive incoming data his socket             */
                    /****************************************************/
                    printf("Descriptor %d is readable\n", i);
                    ssize_t len;
                    memset(buffer, 0, BUFFER_SIZE);
                    if ((len = read(i, buffer, BUFFER_SIZE)) < 0) {
                        delete_memory(pool);
                        perror("\nproblem with read form the socket: from the main() function");
                        exit(-1);
                    }
                    if (len == 0) {//finish to read.
                        /* If the connection has been closed by client 		*/
                        /* remove the connection (remove_conn(...))    		*/
                        printf("Connection closed for sd %d\n", i);
                        //now I want to remove the connection.
                        remove_conn(i, pool);
                    }
                    else{
                        printf("%d bytes received from sd %d\n", (int)len, i);
                        /**********************************************/
                        /* Data was received, add msg to all other    */
                        /* connectios					  			  */
                        /**********************************************/
                        add_msg(i, buffer, (int)len, pool);
                    }
                    //			} /* End of if (FD_ISSET()) */
                    /*******************************************************/
                    /* Check to see if this descriptor is ready for write  */
                    /*******************************************************/
                }
            }
            if(FD_ISSET(i, &(pool->ready_write_set))){
                /* try to write all msgs in queue to sd */
                write_to_client(i,pool);
            }
            if(pool->nready == 0)
                break;
        }

    } while (end_server == 0); /* End of loop through selectable descriptors */
	/*************************************************************/
	/* If we are here, Control-C was typed,						 */
	/* clean up all open connections					         */
	/*************************************************************/
    delete_memory(pool);
    close(main_socket);
	return 0;
}

/*
 * Add connection when new client connects the server.
 * @ sd - the socket descriptor returned from accept
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */
int init_pool(conn_pool_t* pool) {
    if(pool==NULL ){
        return -1;
    }
	//initialized all fields
    pool->maxfd=0;
    pool->nready=0;
    FD_ZERO(&(pool->read_set));
    FD_ZERO(&(pool->ready_read_set));
    FD_ZERO(&(pool->write_set));
    FD_ZERO(&(pool->ready_write_set));
    /* Doubly-linked list of active client connection objects. */
    pool->conn_head=NULL;
    /* Number of active client connections. */
    pool->nr_conns=0;
    return 0;
}

/*
 * Add connection when new client connects the server.
 * @ sd - the socket descriptor returned from accept
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */
int add_conn(int sd, conn_pool_t* pool) {
	/*
	 * 1. allocate connection and init fields
	 * 2. add connection to pool
	 * */
    if(pool==NULL || sd<0){
        return -1;
    }

    conn_t *ptr_curr=NULL;
    conn_t *ptr_prev=NULL;
    conn_t *connection=(conn_t*) malloc(sizeof(conn_t));//allocate the connection
    if(connection == NULL){
        delete_memory(pool);
        perror("\nfail with malloc connection at function: add_conn\n");
        return -1;
    }
    //add connection to the pool.
    connection->fd=sd;
    connection->write_msg_head=NULL;
    connection->write_msg_tail=NULL;
    connection->next=NULL;
    connection->prev=NULL;
    /*if the list is empty. */
    if((pool->conn_head) ==NULL){
        pool->conn_head=connection;
    }
    /* new node will be added after the tail*/
    else{
        ptr_curr=pool->conn_head;
        while(ptr_curr){
            ptr_prev=ptr_curr;
            ptr_curr=ptr_curr->next;
        }
        ptr_prev->next=connection;
        connection->prev=ptr_prev;
    }
    FD_SET(sd,&(pool->read_set));

    for(int i=0 ;i< 1000; i++){
        if( array[i]==0){
            array[i]=sd;
            max_fd_array(pool);//update the max fd val.
            break;
        }
    }
    (pool->nr_conns)++;
	return 0;
}

/*
 * Add msg to the queues of all connections (except of the origin).
 * @ sd - the socket descriptor to add this msg to the queue in its conn object
 * @ buffer - the msg to add
 * @ len - length of msg
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */
int add_msg(int sd, char* buffer, int len, conn_pool_t* pool) {
    /*
     * 1. add msg_t to write queue of all other connections
     * 2. set each fd to check if ready to write
     */
    if(pool==NULL || sd<0){
        return -1;
    }
    conn_t *ptr_pool = pool->conn_head;
    msg_t *ptr_msg;

    while (ptr_pool) {
        if (sd != ptr_pool->fd) {
            ptr_msg = (msg_t*) malloc(sizeof(msg_t));
            if (ptr_msg == NULL) {
                perror("Error with malloc ptr_msg, from function: add_msg()");
                return -1;
            }

            ptr_msg->message = malloc(sizeof(char) * (len + 1));
            if ((ptr_msg->message) == NULL) {
                free(ptr_msg);
                perror("Error with malloc ptr_msg, from function: add_msg()");
                return -1;
            }

            strncpy(ptr_msg->message, buffer, len);
            ptr_msg->size = len;

            if (ptr_pool->write_msg_head == NULL) {
                ptr_msg->prev = ptr_msg->next = NULL;
                ptr_pool->write_msg_head = ptr_pool->write_msg_tail = ptr_msg;
            } else {
                ptr_msg->prev = ptr_pool->write_msg_tail;
                ptr_msg->next = NULL;
                ptr_pool->write_msg_tail->next = ptr_msg;
                ptr_pool->write_msg_tail = ptr_msg;
            }

            FD_SET(ptr_pool->fd, &(pool->write_set));
        }

        ptr_pool = ptr_pool->next;
    }
    FD_CLR(sd,&(pool->write_set));

    return 0;
}

/*
 * Write msg to client.
 * @ sd - the socket descriptor of the connection to write msg to
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */
int write_to_client(int sd,conn_pool_t* pool) {
    /*
     * 1. write all msgs in queue
     * 2. deallocate each writen msg
     * 3. if all msgs were writen successfully, there is nothing else to write to this fd... */
    if(pool==NULL || sd<0){
        return -1;
    }
    conn_t *pool_ptr=pool->conn_head;
    msg_t *msg_ptr;
    msg_t *msg_to_alloc;
    ssize_t written;
    while(pool_ptr){
        if(pool_ptr->fd==sd){
            msg_ptr=pool_ptr->write_msg_head;
            while(msg_ptr){

                if((written= write(sd, msg_ptr->message, msg_ptr->size) )< 0) {
                    delete_memory(pool);
                    perror("Error writing to client");
                    return -1;
                }
                if(written < msg_ptr->size) {
                    memmove(msg_ptr->message, msg_ptr->message + written, msg_ptr->size - written);
                    msg_ptr->size -= (int)written;
                    break;
                } else {
                    msg_to_alloc = msg_ptr;
                    msg_ptr = msg_ptr->next;
                    if (msg_to_alloc == pool_ptr->write_msg_head) {
                        pool_ptr->write_msg_head = msg_ptr;
                    } else {
                        msg_to_alloc->prev->next = msg_ptr;
                    }
                    if (msg_to_alloc == pool_ptr->write_msg_tail) {
                        pool_ptr->write_msg_tail = msg_to_alloc->prev;
                    }
                    if (msg_ptr) {
                        msg_ptr->prev = msg_to_alloc->prev;
                    }
                    free(msg_to_alloc->message);
                    free(msg_to_alloc);
                }
            }

            if(!pool_ptr->write_msg_head) {
                FD_CLR(sd, &(pool->write_set));
            }
            return 0;
        }
        pool_ptr=pool_ptr->next;
    }
    return -1;
}

/*
 * Remove connection when a client closes connection, or clean memory if server stops.
 * @ sd - the socket descriptor of the connection to remove
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */
int remove_conn(int sd, conn_pool_t* pool) {
    /*
    * 1. remove connection from pool
    * 2. deallocate connection
    * 3. remove from sets
    * 4. update max_fd if needed
    */
    if(pool==NULL || sd<0){
        return -1;
    }
    conn_t *ptr = pool->conn_head;
    conn_t *prev = NULL;
    while (ptr != NULL) {
        if (ptr->fd == sd) {
            printf("removing connection with sd %d \n", sd);

            // remove connection from pool
            if (prev != NULL) {
                prev->next = ptr->next;
            }
            else {
                pool->conn_head = ptr->next;
            }
            if (ptr->next != NULL) {
                ptr->next->prev = prev;
            }
            //Update the max fd if this sd = maxfd
            remove_messages(ptr->write_msg_head);
            FD_CLR(sd, &pool->read_set);
            FD_CLR(sd, &pool->write_set);
            free(ptr);
            for(int i=0 ;i<1000 ; i++){
                if(array[i]==sd){
                    array[i]=0;
                    break;
                }
            }
            max_fd_array(pool);
            close(sd);
            (pool->nr_conns)--;
            return 0 ;
        }
        prev = ptr;
        ptr = ptr->next;
    }
    return -1;
}

//-------------------check_if_string_is_number-----------------
bool isnumber (char* str){
    if(str==NULL)
        return false;
    for(int i=0; i<(int)strlen(str);i++)
        if(!isdigit(str[i]))
            return false;
    return true;
}
//-------------------check_if_string_is_number-----------------
//-------------------remove_a_specific_message-----------------
void remove_messages(msg_t * m){
    msg_t *ptr=m;
    while (ptr){
        free(ptr->message);
        ptr->message=NULL;
        ptr->size=0;
        ptr=m->next;
    }
}
//-------------------remove_a_specific_message-----------------
//-------------------free_all----------------------------------
void delete_memory(conn_pool_t* pool){
    if(pool==NULL ){
        return;
    }
    conn_t *ptr_curr=pool->conn_head;
    conn_t *ptr_next=NULL;
    while(ptr_curr){
        ptr_next=ptr_curr->next;
        remove_conn(ptr_curr->fd,pool);
        ptr_curr=ptr_next;
    }
    free(pool);
    pool=NULL;
}
//-------------------free_all----------------------------------
//-------------------update_the_maxfd_from_the_global_array----
void max_fd_array( conn_pool_t* pool ){
    if(pool==NULL){
        return;
    }
    int max =3;
    for (int i = 0; i < 999; i++) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    pool->maxfd=max;
}
//-------------------update_the_maxfd_from_the_global_array----

