/*
  Proxy.cpp

*/


#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <iostream>

void* to_client_thread(void* arg);
void* to_server_thread(void* arg);
int bankSocket; //Global FDs for network sockets
int atmSocket;
unsigned short bankPortNo;

struct thread_info {    		/* Used as argument to thread_start() */
	pthread_t thread_id;     /* ID returned by pthread_create() */
	//int arg;
	//int bankSocket;
	int csock;
	int bsock;
};

void closeSocket(int param) {
	printf("\nShutting down...\n");
	fflush(stdout);
	close(bankSocket);
	close(atmSocket);
	exit(1);
}

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("Usage: proxy <listen-port> <bank-connect-port>\n");
		return -1;
	}

	//Close sock on Ctrl-C
	signal(SIGINT, closeSocket);

  // Parse Arguments for ports
	unsigned short atmPortNo = atoi(argv[1]);
	bankPortNo = atoi(argv[2]);

  // Create a socket(New Connection Endpoint)
  // ARG 0, AF_INET - Adress Domain
  // ARG 1, SOCK_STREAM -  Type of socket (This is a stream)
  // ARG 2, Protocol (zero means the OS will choose the most appropriate)
	atmSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(atmSocket == -1) {
		printf("Socket Not Created\n");
		return -1;
	}

  /*
    sockaddr_in is a structure containing an internet address (defined in netinet/in.h)
    struct sockaddr_in
    {
      short   sin_family;   // must be AF_INET
      u_short sin_port;
      struct  in_addr sin_addr;
      char    sin_zero[8];  // Not used, must be zero
    };
  */
	sockaddr_in addr_l;
	addr_l.sin_family = AF_INET;
	addr_l.sin_port = htons(atmPortNo);
	addr_l.sin_addr.s_addr = INADDR_ANY;

	if (bind(atmSocket, (struct sockaddr *) &addr_l, sizeof(addr_l)) < 0){
		error("ERROR on binding");
	}

	listen(atmSocket,SOMAXCONN);

	//loop forever accepting new connections
	pthread_t thread_id = 0;
	while(1) {
		thread_info t;
		sockaddr_in unused;
		socklen_t size = sizeof(unused);
		t.csock = accept(atmSocket, reinterpret_cast<sockaddr*>(&unused), &size);
		if(t.csock < 0)	//bad client, skip it
			continue;
		t.bsock = socket(AF_INET, SOCK_STREAM, 0);
		if(t.bsock < 0)	//bad server, skip it
			continue;

			
		sockaddr_in addr_b;
		addr_b.sin_family = AF_INET;
		addr_b.sin_port = htons(bankPortNo);
		addr_b.sin_addr.s_addr = INADDR_ANY;

		if(0 != connect(t.bsock, reinterpret_cast<sockaddr*>(&addr_b), sizeof(addr_b))) 
			continue;
		//t.arg = csock;
		//t.bankSocket = bankSocket;
		t.thread_id = thread_id++;

		pthread_t thread;

		pthread_create(&thread, NULL, to_client_thread, &t);
		t.thread_id = thread_id++;
		pthread_create(&thread, NULL, to_server_thread, &t);
	}
}

void* to_server_thread(void* arg) {
	struct thread_info *tinfo;
	tinfo = (thread_info *) arg;
	int csock = tinfo->csock;

	printf("[proxy] client ID #%d connected\n", csock);

	// New socket to connect to the bank
	int bsock = tinfo->bsock;

	/*if(bsock == -1) {
		printf("Socket Not Created\n");
		pthread_exit(NULL);
	}

	sockaddr_in addr_b;
	addr_b.sin_family = AF_INET;
	addr_b.sin_port = htons(bankPortNo);
	addr_b.sin_addr.s_addr = INADDR_ANY;

	if(0 != connect(bsock, reinterpret_cast<sockaddr*>(&addr_b), sizeof(addr_b))) {
		printf("fail to connect to bank\n");
		pthread_exit(NULL);
	}*/

	//input loop
	unsigned int length;
	char packet[1024];
	while(1) {
		//read the packet from the ATM
    // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    /*
    s [in] The descriptor that identifies a connected socket.
    buf [out] A pointer to the buffer to receive the incoming data.
    len [in] The length, in bytes, of the buffer pointed to by the buf parameter.
    flags [in] A set of flags that influences the behavior of this function.
    */

	  int n;
	  char buffer[256];

	  bzero(buffer,256);
	  n = read(csock,buffer,255);
	  std::cout << "Client: " << buffer << std::endl;
	  if (n < 0) error("ERROR reading from socket");
	  if (n == 0) break;

    /*
      Other Team may mess with code here.
      Good Luck!
    */

		n = write(bsock,buffer,n);
		if (n < 0) error("ERROR writing to socket");
		if (n == 0) break;


		bzero(buffer,256);
		//n = read(bankSocket,buffer,255);
		//std::cout << "Server: " << buffer << std::endl;
		//if (n < 0) error("ERROR reading from socket");

		/*
      Other Team may mess with code here.
      Good Luck!
    */

		//n = write(csock,buffer,n);
		//if (n < 0) error("ERROR writing to socket");

	}

	printf("Disconected from client %d \n", csock);

	close(bsock);
	close(csock);
	pthread_exit(NULL);
}

/*
  Create a client thread on new socket connection
*/

void* to_client_thread(void* arg) {
	struct thread_info *tinfo;
	tinfo = (thread_info *) arg;
	int csock = tinfo->csock;
	printf("[proxy] client ID #%d connected\n", csock);

	// New socket to connect to the bank
	// bankSocket = socket(AF_INET, SOCK_STREAM, 0);
	int bsock = tinfo->bsock;

	/*if(bsock == -1) {
    printf("Socket Not Created\n");
		pthread_exit(NULL);
	}

	sockaddr_in addr_b;
	addr_b.sin_family = AF_INET;
	addr_b.sin_port = htons(bankPortNo);
	addr_b.sin_addr.s_addr = INADDR_ANY;

	if(0 != connect(bsock, reinterpret_cast<sockaddr*>(&addr_b), sizeof(addr_b))) {
		printf("fail to connect to bank\n");
		pthread_exit(NULL);
	}*/

	//input loop
	unsigned int length;
	char packet[1024];
	while(1) {
		//read the packet from the ATM
    // ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    /*
    s [in] The descriptor that identifies a connected socket.
    buf [out] A pointer to the buffer to receive the incoming data.
    len [in] The length, in bytes, of the buffer pointed to by the buf parameter.
    flags [in] A set of flags that influences the behavior of this function.
    */

	  int n;
	  char buffer[256];

	  //bzero(buffer,256);
	  //n = read(csock,buffer,255);
	  //std::cout << "Client: " << buffer << std::endl;
	  //if (n < 0) error("ERROR reading from socket");

    /*
      Other Team may mess with code here.
      Good Luck!
    */


		//n = write(bankSocket,buffer,n);
		//if (n < 0) error("ERROR writing to socket");

		bzero(buffer,256);
		n = read(bsock,buffer,255);
		std::cout << "Server: " << buffer << std::endl;
		if (n < 0) error("ERROR reading from socket");
		if (n == 0) break;

		/*
      Other Team may mess with code here.
      Good Luck!
    */
		n = write(csock,buffer,n);
		if (n < 0) error("ERROR writing to socket");
		if (n == 0) break;

	}

	printf("Disconected from client %d \n", csock);

	close(bsock);
	close(csock);
	pthread_exit(NULL);
}
