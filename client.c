#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "socket.h"

#define BUFFER_LEN 256


int main(int argc, char** argv) {
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <server name> <port>\n", argv[0]);
    exit(1);
  }
	
  // Read command line arguments
  char* server_name = argv[1];
  unsigned short port = atoi(argv[2]);
	
  // Connect to the server
  int socket_fd = socket_connect(server_name, port);
  if(socket_fd == -1) {
    perror("Failed to connect");
    exit(2);
  }
  
  // Set up file streams
  FILE* to_server = fdopen(dup(socket_fd), "wb");
  if(to_server == NULL) {
    perror("Failed to open stream to server");
    exit(2);
  }
  
  FILE* from_server = fdopen(dup(socket_fd), "rb");
  if(from_server == NULL) {
    perror("Failed to open stream from server");
    exit(2);
  }

  char msg[BUFFER_LEN];
  while(1) {
    fgets(msg, BUFFER_LEN, stdin);

    //strip ending newline
    
    
    if(strcmp("quit\n", msg)==0) {
      break;
    }

    // Send a message to the server
    fprintf(to_server, "%s", msg);
  
    // Flush the output buffer
    fflush(to_server);

    // Read a message from the server
    char buffer[BUFFER_LEN];
    if(fgets(buffer, BUFFER_LEN, from_server) == NULL) {
      perror("Reading from server failed");
      exit(2);
    }
  
    printf("Server sent: %s", buffer);
  }
	
  // Close file streams
  fclose(to_server);
  fclose(from_server);
  
  // Close socket
  close(socket_fd);
	
  return 0;
}


















