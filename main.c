#include "config.h"
#include "server.h"

/**
 * main()
 *
 * Entry point into our program. Parse the command line parameters and start
 * the server as required.
 *
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 **/
int main(int argc, char** argv){
  /* Setup default values for variables */
  int port = PORT;
  int maxRead = MAX_READ;
  /* TODO: Parse the command line parameters. */
  /* Initialise the server */
  server_init(port, maxRead);
  /* Run main server loop */
  while(1){
    server_service();
  }
}
