# Server-Client-Chat

Local Client/Server Chatroom Program

# Features

  - Multiple communicating processes: clients to servers
  - Communication through FIFOS
  - Typed input (client) and server info handled by using multiple threads in the client program.
  - Server shutdown using signal handling
  - input multiplexing with select()

# Usage

Compilation:
  - Once source code is downloaded, change to the directory and use the command 'make' to compile everything needed.
  
Server:
  - User starts bl-server in one terminal to manage the chatroom. Server is non-interactive, only printing debugging output while ran
  - Command: ./bl_server server_name

Client:
  - Users wanting to chat run bl-client in another terminal which takes user input and broadcasts the input to the server
  - The server then broadcasts the input to all the other clients
  - Spin up as many clients as wanted in separate terminals
  - Command: ./bl_client server_name client_name
  
# Demo

![client-server_Demo](https://user-images.githubusercontent.com/60115853/92154140-f1052d80-edea-11ea-8383-cf3c25655264.png)







