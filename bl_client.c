#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t user_thread;
pthread_t server_thread;

void *user_worker(void *arg){
  // Creation of message to be filled out by the input loop below
  mesg_t message = {};
  strcpy(message.name, client->name);
  message.kind = BL_MESG;

  while(!simpio->end_of_input){
    simpio_reset(simpio);
    // Ensures cooperation with input being typed
    iprintf(simpio, "");
    while(!simpio->line_ready && !simpio->end_of_input){
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){
      // Creates message with line and writes to to_server
      strcpy(message.body, simpio->buf);
      write(client->to_server_fd, &message, sizeof(mesg_t));
    }
  }
  // Writing DEPARTED mesg_t into to_server
  mesg_t departed_message = {};
  departed_message.kind = BL_DEPARTED;
  snprintf(departed_message.name,MAXNAME, "%s", message.name);
  write(client->to_server_fd, &departed_message, sizeof(mesg_t));

  // Cancel server thread
  pthread_cancel(server_thread);
  return NULL;
}

void *server_worker(void *arg){

  mesg_t message = {};
  while(1){
    // Read a mesg_t from to_client FIFO
    //iprintf(simpio, "client name: %s\n", client->name);
    //iprintf(simpio, "to_client_fd: %d\n", client->to_client_fd);
    //iprintf(simpio, "to_server_fd: %d\n", client->to_server_fd);
    //iprintf(simpio, "to_client_fname: %s\n", client->to_client_fname);
    //iprintf(simpio, "to_server_fname: %s\n", client->to_server_fname);
    //iprintf(simpio, "data_ready: %d\n", client->data_ready);
    read(client->to_client_fd, &message, sizeof(mesg_t));
    //iprintf(simpio, "Read return value: %d\n", bytes_read);
    //iprintf(simpio, "Server thread: after write reading from to_client_fd: mesg kind: %d from %s -%s \n", message.kind, message.name, message.body);

    // Print appropriate response to terminal with simpio
    if (message.kind == BL_MESG){
      iprintf(simpio, "[%s] : %s\n", message.name, message.body);
    }

    else if (message.kind == BL_JOINED){
      iprintf(simpio, "-- %s JOINED --\n", message.name);
    }

    else if (message.kind == BL_DEPARTED){
      iprintf(simpio, "-- %s DEPARTED --\n", message.name);
    }
    else if (message.kind == BL_SHUTDOWN){
      iprintf(simpio, "!!! server is shutting down !!!\n");
      // Break out of loop if BL_SHUTDOWN mesg_t is read
      break;
    }
  }
  // Cancel user thread
  pthread_cancel(user_thread);
  return NULL;
}

int main(int argc, char *argv[]){
  char name[MAXNAME];
  char server_fifo[MAXPATH];
  char to_server_fifo[MAXPATH];
  char to_client_fifo[MAXPATH];
  char *server_tag = ".fifo";
  char *to_server_tag = "_server.fifo";
  char *to_client_tag = "_client.fifo";

  // Naming to_client and to_server fifos "pid_client.fifo" and "pid_server.fifo" respectively
  snprintf(to_server_fifo,MAXPATH, "%d%s", getpid(),to_server_tag);
  snprintf(to_client_fifo,MAXPATH, "%d%s", getpid(),to_client_tag);

  // Naming server fifo "server_name.fifo" uses name of server stored in argv[1]
  snprintf(server_fifo ,MAXPATH, "%s%s", argv[1],server_tag);

  // Copies name of user stored in argv[2]
  strcpy(name,argv[2]);

  // Make and open to_server and to_client fifos, initialize client data
  mkfifo(to_server_fifo, DEFAULT_PERMS);
  mkfifo(to_client_fifo, DEFAULT_PERMS);
  client->to_server_fd = open(to_server_fifo,O_RDWR);
  client->to_client_fd = open(to_client_fifo,O_RDWR);
  client->data_ready = 0;
  snprintf(client->to_client_fname, MAXPATH, "%s", to_client_fifo);
  snprintf(client->to_server_fname, MAXPATH, "%s", to_server_fifo);
  snprintf(client->name, MAXPATH, "%s", name);


  // Create join request
  join_t join_request = {};
  strcpy(join_request.name,name);
  strcpy(join_request.to_client_fname, to_client_fifo);
  strcpy(join_request.to_server_fname, to_server_fifo);

  // Open and write join request to server fifo
  int join_fd = open(server_fifo, O_RDWR);
  write(join_fd, &join_request, sizeof(join_t));

  // Set prompt, initialize io, and set terminal into a compatible mode
  char prompt[MAXNAME];
  snprintf(prompt, MAXNAME, "%s>>",name);
  simpio_set_prompt(simpio, prompt);
  simpio_reset(simpio);
  simpio_noncanonical_terminal_mode();

  // Start user thread and server thread
  pthread_create(&user_thread,   NULL, user_worker,   NULL);
  pthread_create(&server_thread, NULL, server_worker, NULL);
  pthread_join(user_thread, NULL);
  pthread_join(server_thread, NULL);

  // Restore standard terminal output
  simpio_reset_terminal_mode();
  return 0;
}
