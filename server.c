#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "util.h"

#ifndef SOL_TCP
  #define SOL_TCP IPPROTO_TCP
#endif

/* Private function declaration */
int server_readFromClient(int fd, int maxRead);
char* server_fdToIp(int fd);
void server_writeToAll_start(int efd);
void server_writeToAll(int efd, char* msg);
void server_writeToAll_end(int efd);
void server_writeToClient_start(int fd);
void server_writeToClient(int fd, char* msg);
void server_writeToClient_end(int fd);
void server_writeNoticeAll(int efd, char* notice, char* data);
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
        int timeout = TIMEOUT_MS;
        setsockopt(new, SOL_TCP, TCP_USER_TIMEOUT, (char*)&timeout, sizeof(timeout));
        FD_SET(new, &active_fd_set);
        server_writeToClient(new, MSG_BANNER);
        /* Send to all connected clients */
        server_writeNoticeAll(new, "Connected", server_fdToIp(new));
      }else{
        /* Data arriving on an already-connected socket */
        if(server_readFromClient(i, maxClientRead) < 0){
          /* Finally disconnected client */
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
  }else if(n == 0){
    /* End-of-file */
    return -1;
  }else{
    /* Check for commands */
    if(buff[0] == '/'){
      switch(buff[1]){
        case 'b' :
          server_writeToClient(fd, MSG_BANNER);
          break;
        case 'h' :
        case '?' :
          server_writeToClient(fd, MSG_HELP);
          break;
        case 'l' :
          server_writeToClient_start(fd);
          server_writeToClient(fd, MSG_PRE);
          server_writeToClient(fd, "IP list:\n");
          for(int i = 0; i < FD_SETSIZE; ++i){
            if(FD_ISSET(i, &active_fd_set)){
              if(i >= 0 && i != socketfd){
                server_writeToClient(fd, MSG_PRE);
                server_writeToClient(fd, "- ");
                server_writeToClient(fd, server_fdToIp(i));
                server_writeToClient(fd, "\n");
              }
            }
          }
          server_writeToClient_end(fd);
          break;
        case 'q' :
          server_writeToClient(fd, MSG_EXIT);
          server_disconnectClient(fd);
          break;
        default :
          /* Do nothing */
          break;
      }
    /* If not a command, relay message to other users */
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
      char* ip = server_fdToIp(fd);
      fprintf(stderr, "%s%s: %s\n", MSG_PRE, ip, buff);
      /* Send to all connected clients */
      server_writeToAll_start(fd);
      server_writeToAll(fd, MSG_PRE);
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
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &active_fd_set)){
      if(i >= 0 && i != socketfd && i != efd){
        server_writeToClient_start(i);
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
  for(int i = 0; i < FD_SETSIZE; ++i){
    if(FD_ISSET(i, &active_fd_set)){
      if(i >= 0 && i != socketfd && i != efd){
        server_writeToClient_end(i);
      }
    }
  }
}

/* TODO */
void server_writeToClient_start(int fd){
  int opt = 1;
  /* Set TCP to cork mode and prevent it from sending a packet */
  setsockopt(fd, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
}

/**
 * server_writeToClient()
 *
 * Write a message to the client. NOTE: See server_writeToClient_start() and
 * server_writeToClient_end() if sending multiple messages to a socket.
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

/* TODO */
void server_writeToClient_end(int fd){
  int opt = 0;
  /* Set TCP to cork mode and prevent it from sending a packet */
  setsockopt(fd, SOL_TCP, TCP_CORK, &opt, sizeof(opt));
}

/* TODO */
void server_writeNoticeAll(int efd, char* notice, char* data){
  fprintf(stderr, "%s%s -> %s\n", MSG_PRE, notice, data);
  server_writeToAll_start(efd);
  server_writeToAll(efd, MSG_PRE);
  if(notice != NULL){
    server_writeToAll(efd, notice);
  }
  if(data != NULL){
    server_writeToAll(efd, " -> ");
    server_writeToAll(efd, data);
  }
  server_writeToAll(efd, "\n");
  server_writeToAll_end(efd);
}

/**
 * server_disconnectClient()
 *
 * Disconnect a given client socket.
 *
 * @param fd The socket to be closed.
 **/
void server_disconnectClient(int fd){
  /* Send to all connected clients */
  server_writeNoticeAll(fd, "Disconnected", server_fdToIp(fd));
  /* Finally, flush and disconnect */
  shutdown(fd, SHUT_WR);
  FD_CLR(fd, &active_fd_set);
}
