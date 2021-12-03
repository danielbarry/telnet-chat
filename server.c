#include <netinet/in.h>
//#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <unistd.h>

/* Variable declaration */
int socketfd;

/**
 * server_init()
 *
 * Initialize the server.
 *
 * @param port The port to be binded.
 **/
void server_init(int port){
  struct sockaddr_in name;
  /* Create the socket */
  socketfd = socket(PF_INET, SOCK_STREAM, 0);
  if(socketfd < 0){
    socketfd = -1;
    perror("socket");
    exit(EXIT_FAILURE);
  }
  /* Give the socket a name */
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Bind the socket */
  if(bind(socketfd, (struct sockaddr*)&name, sizeof(name)) < 0){
    socketfd = -1;
    perror("bind");
    exit(EXIT_FAILURE);
  }
  /* Check whether we initialized correctly */
  if(listen(socketfd, 1) < 0){
    socketfd = -1;
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void server_service(){
  /* TODO: Service the clients connecting/connected to the server. */
}
