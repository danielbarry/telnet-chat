#include <arpa/inet.h>
#include <netinet/in.h>
//#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <unistd.h>

#include "server.h"

/* Private function declaration */
int server_readFromClient(int fd, int maxRead);

/* Variable declaration */
int socketfd;
int maxClientRead;
fd_set active_fd_set;
fd_set read_fd_set;

void server_init(int port, int maxRead){
  struct sockaddr_in name;
  maxClientRead = maxRead;
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
  /* Initialise set of active sockets */
  FD_ZERO(&active_fd_set);
  FD_SET(socketfd, &active_fd_set);
}

void server_service(){
  /* Ensure the socket file descriptor is valid */
  if(socketfd < 0){
    socketfd = -1;
    perror("loop");
    exit(EXIT_FAILURE);
  }
  /* Block until input arrives on one or more active sockets */
  read_fd_set = active_fd_set;
  if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0){
    socketfd = -1;
    perror("select");
    exit(EXIT_FAILURE);
  }
  /* Service all the sockets with input pending */
  struct sockaddr_in clientName;
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &read_fd_set)){
      if(i == socketfd){
        /* Connection request on original socket */
        int new;
        socklen_t size = sizeof(clientName);
        new = accept(socketfd, (struct sockaddr*)&clientName, &size);
        if(new < 0){
          socketfd = -1;
          perror("accept");
          exit(EXIT_FAILURE);
        }
        fprintf(
          stderr,
          "Server: connect from host %s, port %hd.\n",
          inet_ntoa(clientName.sin_addr),
          ntohs(clientName.sin_port)
        );
        FD_SET(new, &active_fd_set);
      }else{
        /* Data arriving on an already-connected socket */
        if(server_readFromClient(i, maxClientRead) < 0){
          close(i);
          FD_CLR(i, &active_fd_set);
        }
      }
    }
  }
  /* TODO: Service all the sockets with output data. */
}

/**
 * server_readFromClient()
 *
 * Read data from the client.
 *
 * @param fd The socket to be read from.
 * @param maxRead The maximum data to be read.
 * @return -1 on error, otherwise 0.
 **/
int server_readFromClient(int fd, int maxRead){
  char buff[maxRead + 1];
  int n = read(fd, buff, maxRead);
  if(n < 0){
    /* Read error */
    perror("read");
    exit(EXIT_FAILURE);
  }else if(n == 0){
    /* End-of-file */
    return -1;
  }else{
    /* Sanitize data to ASCII and remove line endings */
    char r = '\0';
    for(int i = n - 1; i >= 0; i--){
      char c = buff[i];
      if(c < ' ' || c > '~'){
        buff[i] = r;
      }else{
        r = ' ';
      }
    }
    buff[n] = '\0';
    /* TODO: Add data to be sent to clients. */
    fprintf(stderr, "Server: got message: '%s'\n", buff);
    return 0;
  }
}
