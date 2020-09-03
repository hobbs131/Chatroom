#include "blather.h"

client_t *server_get_client(server_t *server, int idx){
  // Gets a pointer to the client_t struct at the given index. If the
  // index is beyond n_clients, the behavior of the function is
  // unspecified and may cause a program crash.
  client_t *client = &server->client[idx];
  return client;
}

void server_start(server_t *server, char *server_name, int perms){
  // Initializes and starts the server with the given name. A join fifo
  // called "server_name.fifo" should be created. Removes any existing
  // file of that name prior to creation. Opens the FIFO and stores its
  // file descriptor in join_fd._
  //
  // ADVANCED: create the log file "server_name.log" and write the
  // initial empty who_t contents to its beginning. Ensure that the
  // log_fd is position for appending to the end of the file. Create the
  // POSIX semaphore "/server_name.sem" and initialize it to 1 to
  // control access to the who_t portion of the log.

  //Appending .fifo to server_name
  char fifo_name[MAXNAME];
  char *s = ".fifo";
  snprintf(fifo_name,MAXPATH, "%s%s", server_name,s);
  strcpy(server->server_name,server_name);

  server->n_clients = 0;
  // Remove any existing file of fifo_name, create and open join fifo, and store descriptor in join_fd
  remove(fifo_name);
  mkfifo(fifo_name, DEFAULT_PERMS);
  int join_fd = open(fifo_name, O_RDWR);
  server->join_fd = join_fd;
}

void server_shutdown(server_t *server){
  // Shut down the server. Close the join FIFO and unlink (remove) it so
  // that no further clients can join. Send a BL_SHUTDOWN message to all
  // clients and proceed to remove all clients in any order.
  //
  // ADVANCED: Close the log file. Close the log semaphore and unlink
  // it.

  // Create BL_SHUTDOWN message
  mesg_t message = {};
  message.kind = BL_SHUTDOWN;
  // Close and unlink join FIFO
  close(server->join_fd);
  remove(server->server_name);

  server_broadcast(server, &message);
  // Remove all clients
  for (int i = 0; i < server->n_clients; i++){
    server_remove_client(server,i);
  }
}

int server_add_client(server_t *server, join_t *join){
  // Adds a client to the server according to the parameter join which
  // should have fileds such as name filed in.  The client data is
  // copied into the client[] array and file descriptors are opened for
  // its to-server and to-client FIFOs. Initializes the data_ready field
  // for the client to 0. Returns 0 on success and non-zero if the
  // server as no space for clients (n_clients == MAXCLIENTS).

  // Failure, No space for clients
  if(server->n_clients == MAXCLIENTS){
    return 1;
  }
  server->n_clients += 1;
  client_t client = {};
  client.data_ready = 0;
  // Copy data to client from join request
  strcpy(client.name,join->name);
  strcpy(client.to_client_fname,join->to_client_fname);
  strcpy(client.to_server_fname,join->to_server_fname);
  // Open file descriptors for to_server and to_client fifos
  client.to_client_fd = open(client.to_client_fname, O_RDWR);
  client.to_server_fd = open(client.to_server_fname, O_RDWR);
  printf("server_add_client(): to_client_fd: %d\n", client.to_client_fd);
  printf("server_add_client(): to_server_fd: %d\n", client.to_server_fd);
  printf("server_add_client(): to_client_fname: %s\n", client.to_client_fname);
  printf("server_add_client(): to_server_fname: %s\n", client.to_server_fname);
  // Add to client array
  server->client[server->n_clients - 1] = client;
  // Success, return 0
  return 0;

}


int server_remove_client(server_t *server, int idx){
  // Remove the given client likely due to its having departed or
  // disconnected. Close fifos associated with the client and remove
  // them. Shift the remaining clients to lower indices of the client[]
  // preserving their order in the array; decreases n_clients.

  // Close and remove fifos associated with client. Basic error checking
  client_t *client = server_get_client(server,idx);

  int close_to_client = close(client->to_client_fd);

  if (close_to_client < 0){
    perror("Failure to close to_client fifo");
  }
  int close_to_server = close(client->to_server_fd);

  if(close_to_server < 0){
    perror("Failure to close to_server fifo");
  }
  int remove_to_client = remove(client->to_client_fname);

  if(remove_to_client < 0){
    perror("Failure to remove to_client fifo");
  }
  int remove_to_server = remove(client->to_server_fname);

  if(remove_to_server < 0){
    perror("Failure to remove to_server fifo");
  }
  // Client to be put in last array slot after shifting clients
  client_t empty_client = {};

  // Shift remaining clients to lower indices
  for(int i = idx; i < server->n_clients; i++){
    server->client[i] =  server->client[i + 1];
  }
  // Set last position to be an empty client and decrement number of clients.
  server->client[server->n_clients - 1] = empty_client;
  server->n_clients -= 1;
  return 1;
}

int server_broadcast(server_t *server, mesg_t *mesg){
  // Send the given message to all clients connected to the server by
  // writing it to the file descriptors associated with them.
  //
  // ADVANCED: Log the broadcast message unless it is a PING which
  // should not be written to the log

  // Error checking message
  printf("server_broadcast(): mesg kind: %d from %s - %s\n" ,mesg->kind, mesg->name, mesg->body);
  for(int i = 0; i < server->n_clients; i++){
    write(server->client[i].to_client_fd, mesg, sizeof(mesg_t));
    printf("server_broadcast(): client number: %d\n", i);
    printf("server_broadcast(): client name: %s\n", server->client[i].name);
    printf("server_broadcast(): to_client_fd: %d\n", server->client[i].to_client_fd);
    printf("server_broadcast(): to_server_fd: %d\n", server->client[i].to_server_fd);
    printf("server_broadcast(): to_client_fname: %s\n", server->client[i].to_client_fname);
    printf("server_broadcast(): to_server_fname: %s\n" , server->client[i].to_server_fname);
    printf("server_broadcast(): data_ready: %d\n", server->client[i].data_ready);
  }
  // Error checking to see that the message is correctly written to to_client_fd (test 2)
  return 1;
}

void server_check_sources(server_t *server){
  // Checks all sources of data for the server to determine if any are
  // ready for reading. Sets the servers join_ready flag and the
  // data_ready flags of each of client if data is ready for them.
  // Makes use of the poll() system call to efficiently determine
  // which sources are ready.
  struct pollfd pfds[server->n_clients + 1];

  // Set-up for pfds array
  for (int i = 0; i < server->n_clients; i++){
    pfds[i].fd = server->client[i].to_server_fd;
    pfds[i].events = POLLIN;
    pfds[i].revents = 0;
  }
  // last slot is join_fd
  pfds[server->n_clients].fd = server->join_fd;
  pfds[server->n_clients].events = POLLIN;
  pfds[server->n_clients].revents = 0;

  int ret = poll(pfds, server->n_clients + 1, -1);
  if( ret < 0 ){
    perror("poll() failed\n");
  }
    // Loop through checking if any source of data is ready for reading -- if so, set fields accordingly.
  for (int i = 0; i < server->n_clients + 1; i++){
    if(pfds[i].revents & POLLIN){
      // Check if it is last array slot which is checking if someone is trying to join
      if (i == server->n_clients){
        printf("join_ready = 1 -- client attempting to join \n");
        server->join_ready = 1;
      }
      // Otherwise it is a client with data, set field accordingly
      else{
        server->client[i].data_ready = 1;
      }
    }
  }
}

int server_join_ready(server_t *server){
  // Return the join_ready flag from the server which indicates whether
  // a call to server_handle_join() is safe.
  return server->join_ready;
}

int server_handle_join(server_t *server){
  // Call this function only if server_join_ready() returns true. Read a
  // join request and add the new client to the server. After finishing,
  // set the servers join_ready flag to 0.

  // Adds client to server after reading in request
  join_t request = {};
  read(server->join_fd, &request, sizeof(join_t));
  server_add_client(server, &request);
  server->join_ready = 0;

  // Creates message to be broadcast
  mesg_t message = {};
  message.kind = BL_JOINED;
  strcpy(message.name,request.name);
  server_broadcast(server, &message);
  return 1;
}

int server_client_ready(server_t *server, int idx){
  // Return the data_ready field of the given client which indicates
  // whether the client has data ready to be read from it.
  return server->client[idx].data_ready;
}

int server_handle_client(server_t *server, int idx){
  // Process a message from the specified client. This function should
  // only be called if server_client_ready() returns true. Read a
  // message from to_server_fd and analyze the message kind. Departure
  // and Message types should be broadcast to all other clients.  Ping
  // responses should only change the last_contact_time below. Behavior
  // for other message types is not specified. Clear the client's
  // data_ready flag so it has value 0.
  //
  // ADVANCED: Update the last_contact_time of the client to the current
  // server time_sec.

  // Create and process message from client
  mesg_t message = {};
  int bytes_read = read(server->client[idx].to_server_fd, &message, sizeof(mesg_t));
  // Basic eror checking for read()
  if (bytes_read < 0){
    perror("Read failed");
  }
  // Broadcast BL_DEPARTED, remove client first
  if (message.kind == BL_DEPARTED){
    server_remove_client(server, idx);
    server_broadcast(server, &message);
  }
  // Broadcast BL_MESG
  else if (message.kind == BL_MESG){
    server_broadcast(server, &message);
  }
  // Clear clients data_ready flag
  server->client[idx].data_ready = 0;
  return 1;
}

void server_tick(server_t *server);
// ADVANCED: Increment the time for the server

void server_ping_clients(server_t *server);
// ADVANCED: Ping all clients in the server by broadcasting a ping.

void server_remove_disconnected(server_t *server, int disconnect_secs);
// ADVANCED: Check all clients to see if they have contacted the
// server recently. Any client with a last_contact_time field equal to
// or greater than the parameter disconnect_secs should be
// removed. Broadcast that the client was disconnected to remaining
// clients.  Process clients from lowest to highest and take care of
// loop indexing as clients may be removed during the loop
// necessitating index adjustments.

void server_write_who(server_t *server);
// ADVANCED: Write the current set of clients logged into the server
// to the BEGINNING the log_fd. Ensure that the write is protected by
// locking the semaphore associated with the log file. Since it may
// take some time to complete this operation (acquire semaphore then
// write) it should likely be done in its own thread to preven the
// main server operations from stalling.  For threaded I/O, consider
// using the pwrite() function to write to a specific location in an
// open file descriptor which will not alter the position of log_fd so
// that appends continue to write to the end of the file.

void server_log_message(server_t *server, mesg_t *mesg);
// ADVANCED: Write the given message to the end of log file associated
// with the server.
