#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
//#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "util.h"

/* Private function declaration */
int server_readFromClient(int fd, int maxRead);
char* server_fdToIp(int fd);
void server_writeToAll_start(int efd);
void server_writeToAll(int efd, char* msg);
void server_writeToAll_end(int efd);
void server_writeToClient(int fd, char* msg);
void server_disconnectClient(int fd);

/* Variable declaration */
int socketfd;
int maxClientRead = MAX_READ;
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
  socklen_t size = sizeof(clientName);
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &read_fd_set)){
      if(i == socketfd){
        /* Connection request on original socket */
        int new = accept(socketfd, (struct sockaddr*)&clientName, &size);
        if(new < 0){
          socketfd = -1;
          perror("accept");
          exit(EXIT_FAILURE);
        }
        fprintf(
          stderr,
          "Server: connect from host %s::%hd\n",
          inet_ntoa(clientName.sin_addr),
          ntohs(clientName.sin_port)
        );
        FD_SET(new, &active_fd_set);
        server_writeToClient(new, MSG_BANNER);
      }else{
        /* Data arriving on an already-connected socket */
        if(server_readFromClient(i, maxClientRead) < 0){
          server_disconnectClient(i);
        }
      }
    }
  }
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
    /* Decide what to do with data */
    if(buff[0] == '/'){
      switch(buff[1]){
        case 'b' :
          server_writeToClient(fd, MSG_BANNER);
          break;
        case 'h' :
        case '?' :
          server_writeToClient(fd, MSG_HELP);
          break;
        case 'q' :
          server_writeToClient(fd, MSG_EXIT);
          server_disconnectClient(fd);
          break;
        default :
          /* Do nothing */
          break;
      }
    }else if(buff[0] >= ' ' && buff[0] <= '~'){
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
      /* Prepare shared values */
      char tstr[12];
      time_t secs = time(0);
      struct tm* local = localtime(&secs);
      sprintf(tstr, "[%02d:%02d:%02d] ", local->tm_hour, local->tm_min, local->tm_sec);
      char* ip = server_fdToIp(fd);
      fprintf(stderr, "%s%s: %s \n", tstr, ip, buff);
      /* Send to all connected clients */
      server_writeToAll_start(fd);
      server_writeToAll(fd, tstr);
      server_writeToAll(fd, ip);
      server_writeToAll(fd, ": ");
      server_writeToAll(fd, buff);
      server_writeToAll(fd, "\n");
      server_writeToAll_end(fd);
    }
    return 0;
  }
}

char* server_fdToIp(int fd){
  struct sockaddr_in clientName;
  socklen_t size = sizeof(clientName);
  if(getpeername(fd, (struct sockaddr*)&clientName, &size) < 0){
    perror("socket_info");
    return "0.0.0.0";
  }
  return inet_ntoa(clientName.sin_addr);
}

/**
 * server_writeToAll_start()
 *
 * Begin a corked write to all active sockets.
 *
 * @param efd An excluded socket file descriptor to not write to. Set to -1 if
 * you want the message to be sent to all sockets.
 **/
void server_writeToAll_start(int efd){
  int opt = 1;
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &active_fd_set)){
      if(i >= 0 && i != socketfd && i != efd){
        /* Set TCP to cork mode and prevent it from sending a packet */
        setsockopt(i, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
      }
    }
  }
}

/**
 * server_writeToAll()
 *
 * Begin writing a packet to the client. NOTE: Messages are buffered in kernel
 * space.
 *
 * @param efd An excluded socket file descriptor to not write to. Set to -1 if
 * you want the message to be sent to all sockets.
 **/
void server_writeToAll(int efd, char* msg){
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &active_fd_set)){
      if(i >= 0 && i != socketfd && i != efd){
        server_writeToClient(i, msg);
      }
    }
  }
}

/**
 * server_writeToAll_end()
 *
 * Close a corked write to all active sockets, causing a flush write.
 *
 * @param efd An excluded socket file descriptor to not write to. Set to -1 if
 * you want the message to be sent to all sockets.
 **/
void server_writeToAll_end(int efd){
  int opt = 0;
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &active_fd_set)){
      if(i >= 0 && i != socketfd && i != efd){
        /* Set TCP to cork mode and prevent it from sending a packet */
        setsockopt(i, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
      }
    }
  }
}

/**
 * server_writeToClient()
 *
 * Write a message to the client. NOTE: See server_writeToClientStart() and
 * server_writeToClientEnd() if sending multiple messages to a socket.
 *
 * @param fd The socket to be written to.
 * @param msg The message to be sent.
 **/
void server_writeToClient(int fd, char* msg){
  /* Send the data */
  if(send(fd, msg, util_strLen(msg), 0) < 0){
    perror("send");
  }
}

/**
 * server_disconnectClient()
 *
 * Disconnect a given client socket.
 *
 * @param fd The socket to be closed.
 **/
void server_disconnectClient(int fd){
  shutdown(fd, SHUT_WR);
  FD_CLR(fd, &active_fd_set);
}
