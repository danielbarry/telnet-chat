/**
 * server_init()
 *
 * Initialize the server.
 *
 * @param port The port to be binded.
 * @param maxRead The maximum data to be read from a client.
 **/
void server_init(int port, int maxRead);

/**
 * server_service()
 *
 * Service the clients connected to the server.
 **/
void server_service();
