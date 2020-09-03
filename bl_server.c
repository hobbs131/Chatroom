#include "blather.h"

int control_variable = 1;

// Signal handler for sigint
void handle_SIGINT(int sig_num) {
  control_variable = 0;
}
// Signal hander for sigterm
void handle_SIGTERM(int sig_num) {
  control_variable = 0;
}
int main(int argc, char *argv[]){

  // Signal handling set up
  struct sigaction my_sa = {};
  sigemptyset(&my_sa.sa_mask);
  my_sa.sa_flags = SA_RESTART;

  my_sa.sa_handler = handle_SIGTERM;
  sigaction(SIGTERM, &my_sa, NULL);

  my_sa.sa_handler = handle_SIGINT;
  sigaction(SIGINT,  &my_sa, NULL);

  // Create server to be passed into server_start
  server_t server = {};
  server_t *server_actual = &server;


  printf("STARTING SERVER\n");
  server_start(server_actual, argv[1], DEFAULT_PERMS);
  printf("SERVER STARTED\n");
  while(control_variable){
    printf("STARTING SERVER_CHECK_SOURCES\n");
    server_check_sources(server_actual);
    printf("SOURCES CHECKED\n");
    if (server_join_ready(server_actual)){
      printf("HANDLING JOIN\n");
      server_handle_join(server_actual);
      printf("JOIN HANDLED\n");
    }
    for (int i = 0; i < server.n_clients; i++){
      if (server_client_ready(server_actual, i)){
        printf("HANDLING CLIENT\n");
        server_handle_client(server_actual,i);
        printf("CLIENT HANDLED\n");
      }
    }
  }
  printf("SHUTTING DOWN SERVER\n");
  server_shutdown(server_actual);
  printf("SERVER SHUTDOWN\n");
  return 0;
}
