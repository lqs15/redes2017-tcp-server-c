#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_NAME_SIZE 50
#define MAX_CANDIDATES 50
#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

typedef struct _candidato {
    int codigo_votacao;
    char nome_candidato[MAX_NAME_SIZE];
    char partido[MAX_NAME_SIZE];
    int num_votos;
} candidato;

void *connection_handler(void *);

int main (int argc, char *argv[]) {

  if (argc < 2) on_error("Usar: %s [port]\n", argv[0]);

  int port = atoi(argv[1]);

  int server_fd, client_fd, err;
  struct sockaddr_in server, client;
  char buf[BUFFER_SIZE];

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) on_error("Could not create socket\n");

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

  err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
  if (err < 0) on_error("Could not bind socket\n");

  err = listen(server_fd, 128);
  if (err < 0) on_error("Could not listen on socket\n");

  printf("Server is listening on %d\n", port);

   socklen_t client_len = sizeof(client);
   while((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len))) {
        puts("Connection accepted");
        pthread_t thread_id; 
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_fd) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_fd < 0)
    {
        perror("accept failed");
        return 1;
    }
  return 0;
}

void *connection_handler(void *socket_desc) {
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size, *opcode;
    char *message , client_message[2000];
     
    //Send some messages to the client
    message = "Digite o código de operação: 111 para receber lista de candidatos e 999 pra enviar lista de votos:\n";
    write(sock , message , strlen(message));
     
    //Receive a message from client
    while((read_size = recv(sock , opcode , sizeof(int) , 0)) > 0)
    {
        write(sock, opcode, sizeof(int));
		//clear the message buffer
		memset(opcode, 0, sizeof(int));
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
}
