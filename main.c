#include "config.h"

/* Function declaration */
void server_init(int port);
void server_service();

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
  /* TODO: Parse the command line parameters. */
  /* Initialise the server */
  server_init(port);
  /* Run main server loop */
  while(1){
    server_service();
  }
}
