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

pthread_mutex_t travaThread;

typedef struct _candidato {
    int codigo_votacao;
    char nome_candidato[MAX_NAME_SIZE];
    char partido[MAX_NAME_SIZE];
    int num_votos;
} candidato;

typedef struct _urna {
    int votos_brancos;
    int votos_nulos;
} urna;

typedef struct _votosRecebidos {
    int codigo_candidato;
    int votos_recebidos;
} votosRecebidos;

int QTD_CANDIDATOS_TOTAIS;
candidato candidatos[MAX_CANDIDATES];
urna urna_interna;

void *connection_handler(void *);

int parseVotosRecebidos(char vet_char[], votosRecebidos votos_recebidos[],urna *urna_recebida){
	char *cp;
	
	urna_recebida->votos_brancos = atoi(strtok(vet_char, ";"));
	urna_recebida->votos_nulos = atoi(strtok(NULL, ";"));
	cp = strtok(NULL, ";");
	
	int i = 0;
	while (cp != NULL){	
		votos_recebidos[i].codigo_candidato = atoi(cp);
		votos_recebidos[i].votos_recebidos = atoi(strtok(NULL,";"));		
		if (cp != NULL){
			cp = strtok(NULL,";");
			i++;
		}
	}
	return i;
}

int recuperarResultadoArquivo (candidato candidatos[], urna *urna) {
    int i = 0;
    FILE *arqCandidatos, *arqUrna;
    arqCandidatos = fopen("candidatos.txt", "r");
    arqUrna = fopen("urna.txt", "r");

    while(!feof(arqCandidatos)) { //verifica se o arquivo esta no final

	    char tcodigo_votacao[50];
    	char tnome_candidato[50];
    	char tpartido[50];
    	char tnum_votos[50];
        char tnum_nulos[10], tnum_brancos[10];

        fscanf(arqCandidatos,";%[^;]s", tcodigo_votacao);
        fscanf(arqCandidatos,";%[^;]s", tnome_candidato);
        fscanf(arqCandidatos,";%[^;]s", tpartido);
        fscanf(arqCandidatos,";%[^;]s", tnum_votos);
        fscanf(arqUrna,";%[^;]s", tnum_nulos);
        fscanf(arqUrna,";%[^;]s", tnum_brancos);
        
        candidatos[i].codigo_votacao = atoi(tcodigo_votacao);
    	strcpy(candidatos[i].nome_candidato, tnome_candidato);
    	strcpy(candidatos[i].partido, tpartido);
    	candidatos[i].num_votos = atoi(tnum_votos);
        urna->votos_brancos = atoi(tnum_brancos);
        urna->votos_nulos = atoi(tnum_nulos);
    	
    	memset(tcodigo_votacao,0,strlen(tcodigo_votacao));
    	memset(tnome_candidato,0,strlen(tnome_candidato));
    	memset(tpartido,0,strlen(tpartido));
    	memset(tnum_votos,0,strlen(tnum_votos));
	/*    	
    	printf("%d\n", candidatos[i].codigo_votacao);
	printf("%s\n", candidatos[i].nome_candidato);
	printf("%s\n", candidatos[i].partido);
	printf("%d\n", candidatos[i].num_votos);*/

        i++;
    }
    fclose(arqCandidatos);
    fclose(arqUrna);
    return i;
}

int main (int argc, char *argv[]) { 
  //candidato candidatos[MAX_CANDIDATES];
  urna urna;
  int server_fd, client_fd, err;
  struct sockaddr_in server, client;
  char buf[BUFFER_SIZE];
  int i;

  QTD_CANDIDATOS_TOTAIS = recuperarResultadoArquivo(candidatos, &urna);
  printf("Qtde de candidatos:%d\n",QTD_CANDIDATOS_TOTAIS);
  for(i=0; i < QTD_CANDIDATOS_TOTAIS; i++) {
        printf(" Resultados Parciais: \n");
        printf(" Codigo do candidato: %d Nome: %s Partido: %s Numero de votos: %d \n ", candidatos[i].codigo_votacao, candidatos[i].nome_candidato, candidatos[i].partido, candidatos[i].num_votos);
  }

  if (argc < 2) on_error("Usar o comando: %s [port]\n", argv[0]);

  int port = atoi(argv[1]);

  if (pthread_mutex_init(&travaThread, NULL) != 0) {
    printf("Erro ao iniciar semáforo de threads");
    return 1;
  }

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (server_fd < 0) 
    on_error("Could not create socket\n");

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

  err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
  
  if (err < 0) 
    on_error("Não foi possível bindar o socket na porta\n");

  err = listen(server_fd, 10);
  if (err < 0) 
    on_error("Não foi possível escutar o socket na porta\n");

  printf("Servidor da Urna escutando na porta %d\n", port);

   socklen_t client_len = sizeof(client);
   while((client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len))) {
        puts("Nova conexão com cliente aceita");
        pthread_t thread_id; 
        if( pthread_create(&thread_id, NULL, connection_handler, (void*) &client_fd) < 0) {
            perror("Não foi possível criar uma nova thread para o cliente");
            return 1;
        }
    }
     
    if (client_fd < 0) {
        perror("Não foi possível aceitar a conexão com um novo cliente");
        return 1;
    }

  pthread_mutex_destroy(&travaThread);
  return 0;
}

void salvaResultadoArquivo(candidato candidatos[], urna *_urna) {
    int i;
    FILE *arqCandidatos, *arqUrna;
    char registro[BUFFER_SIZE], urna[BUFFER_SIZE];
    arqCandidatos = fopen("candidatos.txt", "w");
    arqUrna = fopen("urna.txt", "w");
    sprintf(urna, ";%d;%d", _urna->votos_nulos, _urna->votos_brancos);
    fprintf(arqUrna, "%s", urna);
    for(i = 0; i < QTD_CANDIDATOS_TOTAIS; i++) {
        sprintf(registro,";%d;%s;%s;%d", candidatos[i].codigo_votacao, candidatos[i].nome_candidato, candidatos[i].partido, candidatos[i].num_votos);
        fprintf(arqCandidatos, "%s", registro);
    }
    fclose(arqCandidatos);
    fclose(arqUrna);
}

void computarVotosRecebidos (int qtd_candidatos_votados, votosRecebidos votos_recebidos[], urna *urna_recebida) {
    int i, j;
    FILE *arqCandidatos, *arqUrna;
    pthread_mutex_lock(&travaThread);
    arqCandidatos = fopen("candidatos.txt", "r");
    arqUrna = fopen("urna.txt", "r");
    urna_interna.votos_brancos += urna_recebida->votos_brancos;
    urna_interna.votos_nulos += urna_recebida->votos_nulos;
    printf("Resultados parciais: \n");
    for(i = 0; i < qtd_candidatos_votados; i++) {
        for (j = 0; j < QTD_CANDIDATOS_TOTAIS; j++) {
            if(candidatos[j].codigo_votacao == votos_recebidos[i].codigo_candidato) {
                candidatos[j].num_votos += votos_recebidos[i].votos_recebidos;
            }
        }
    }
    for(j = 0; j < QTD_CANDIDATOS_TOTAIS; j++) {
        printf("Codigo do candidato: %d Nome: %s Partido: %s Numero de votos: %d \n ", candidatos[j].codigo_votacao, candidatos[j].nome_candidato, candidatos[j].partido, candidatos[j].num_votos);
    }
    salvaResultadoArquivo(candidatos, &urna_interna);
    pthread_mutex_unlock(&travaThread);
}

void *connection_handler(void *socket_desc) {
    //Recebe o ponteiro do socket na função da thread
    int i;
    FILE *arqCandidatos, *arqUrna;
    int sock = *(int*)socket_desc;
    int tam_opcode, opcode, tam_votos_recebidos, qtd_candidatos_votados;
    char *mensagem , opcode_recebido[3];
    char votos_recebidos[BUFFER_SIZE];
    char lista_candidatos[BUFFER_SIZE];
    votosRecebidos votos_urna[MAX_CANDIDATES];
    urna urna_recebida;

    arqCandidatos = fopen("candidatos.txt", "r");
    fgets(lista_candidatos, BUFFER_SIZE, arqCandidatos);

    //Recebendo os códigos do cliente para responder
    while ((tam_opcode = recv(sock, opcode_recebido, 10 , 0)) > 0) {
        opcode = atoi(opcode_recebido);
        if (opcode == 888) {
            puts("Solicitacao de envio de lista recebido. Enviando lista de candidatos para cliente.");
            write(sock, lista_candidatos, strlen(lista_candidatos));
        } else if (opcode == 999) {
           if((tam_votos_recebidos = recv(sock, votos_recebidos, BUFFER_SIZE, 0) > 0)) {
               printf("String recebida: %s\n",votos_recebidos);
               qtd_candidatos_votados = parseVotosRecebidos(votos_recebidos, votos_urna, &urna_recebida);
               printf("Numero Candidatos: %d\n",qtd_candidatos_votados);
               printf("Nulos: %d\n", urna_recebida.votos_nulos);
               printf("Brancos: %d\n", urna_recebida.votos_brancos);
               computarVotosRecebidos(qtd_candidatos_votados, votos_urna, &urna_recebida);
           } 
        } else {
            write(sock, "codigo invalido", 15);
        }
		//clear the message buffer
		memset(opcode_recebido, 0, 10);
    }
     
    if(tam_opcode == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(tam_opcode == -1)
    {
        perror("recv failed");
    }
         
}
