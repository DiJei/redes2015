/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 12/08/2013
 * 
 * Um c�digo simples (n�o � o c�digo ideal, mas � o suficiente para o
 * EP) de um servidor de eco a ser usado como base para o EP1. Ele
 * recebe uma linha de um cliente e devolve a mesma linha. Teste ele
 * assim depois de compilar:
 * 
 * ./servidor 8000
 * 
 * Com este comando o servidor ficar� escutando por conex�es na porta
 * 8000 TCP (Se voc� quiser fazer o servidor escutar em uma porta
 * menor que 1024 voc� precisa ser root).
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequ�ncias de caracteres seguidas de ENTER. Voc� ver� que
 * o telnet exibe a mesma linha em seguida. Esta repeti��o da linha �
 * enviada pelo servidor. O servidor tamb�m exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Voc� pode conectar no servidor remotamente tamb�m. Basta saber o
 * endere�o IP remoto da m�quina onde o servidor est� rodando e n�o
 * pode haver nenhum firewall no meio do caminho bloqueando conex�es na
 * porta escolhida.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096


 /*Estrutura que vai ser usada para armazenar clientes*/
struct no {
   char *nick;   //Tamanho maximo descrito no protocolo
   struct no *prox;
};
typedef struct no node;
/*----------------------------------------------------*/


void insert_client(char *nick);
int check_nick(char *nick);
void getTemperatura(char temp[]);
void imprime_cliente(node *p);
void update_client(node *p, FILE *client);
void get_nick(char *line, char nick[]);



int main (int argc, char **argv) {
   /* Os sockets. Um que ser� o socket que vai escutar pelas conex�es
    * e o outro que vai ser o socket espec�fico de cada conex�o */
	int listenfd, connfd;
   /* Informa��es sobre o socket (endere�o e porta) ficam nesta struct */
	struct sockaddr_in servaddr;
   /* Retorno da fun��o fork para saber quem � o processo filho e quem
    * � o processo pai */
   pid_t childpid;

   /* Armazena linhas recebidas do cliente */
	char	recvline[MAXLINE + 1];
   /* Armazena o tamanho da string lida do cliente */
   ssize_t  n;
   
   /*Para pegar a hora do sistema*/
   /*----------------------------*/
   time_t hora;
   struct tm *tm_p;
   /*----------------------------*/
 
   int x;
   char data[13];
   char hour[14];
   //temperatura
   FILE *file;
   char temp[7];
   char tempsp[39];
   char http[28];
   //------------
   
   FILE *client;
   char* answer;
   /*Fila de usuarios*/
   node *fila_client;
   fila_client = malloc(sizeof(node));
   fila_client->prox = NULL;
   char entrada[30];
   char nick[10];
   int lenfile;

   /* Inicialização de estruturas */

   file = fopen("channels.txt", "w+");
   fclose(file);
   file = fopen("nicks.txt", "w+");
   fclose(file);

  if (argc != 2) {
      fprintf(stderr,"Uso: %s <Porta>\n",argv[0]);
      fprintf(stderr,"Vai rodar um servidor de echo na porta <Porta> TCP\n");
		exit(1);
	}
  
  /*Cria arquivo com informacoes com clientes*/
  
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket :(\n");
		exit(2);
	}


	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind :(\n");
		exit(3);
	}


	if (listen(listenfd, LISTENQ) == -1) {
		perror("listen :(\n");
		exit(4);
	}

   printf("[Servidor no ar. Aguardando conexoes na porta %s]\n",argv[1]);
   printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
   

	for (;;) {
 
		if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
			perror("accept :(\n");
			exit(5);
		}
      
   
      if ( (childpid = fork()) == 0) {
         /**** PROCESSO FILHO ****/
         printf("[Uma conexao aberta]\n");
         /* J� que est� no processo filho, n�o precisa mais do socket
          * listenfd. S� o processo pai precisa deste socket. */
         close(listenfd);
         


         /* ========================================================= */
         /* ========================================================= */
         /*                         EP1 IN�CIO                        */
         /* ========================================================= */
         /* ========================================================= */
         /* TODO: � esta parte do c�digo que ter� que ser modificada
          * para que este servidor consiga interpretar comandos IRC   */
                

         while ((n=read(connfd, recvline, MAXLINE)) > 0) { 
            
            //flush hora
            hora = time( NULL );
            recvline[n]=0;

            if (strncmp(recvline,"LIST",4) == 0) {
               file = fopen("channels.txt", "r+");
               answer = "#global\n";
               write(connfd, answer, strlen(answer));
               answer = "#private\n";
               write(connfd, answer, strlen(answer));
            }

            if (strncmp(recvline,"JOIN",4) == 0) {
                get_nick(recvline,nick);
                printf("%s\n",nick);
                if (strncmp(nick,"#global",7) == 1){
                    printf("JOIN GLOBAL\n");
                }
                if (strncmp(nick,"#private",8) == 1){
                  printf("JOIN PRIVATE\n");
                }
                else {
                  answer = "ERR_NOSUCHCHANNEL\n";
                  write(connfd, answer, strlen(answer)); 
                }
            } 
          
            if (strncmp(recvline,"NICK",4) == 0) {
              get_nick(recvline,nick);
              if (check_nick(nick) == 1) {
                  answer = "433 ERR_NICKNAMEINUSE\0";
                  write(connfd, answer, strlen(answer));
              }
              else {
                  insert_client(nick);
                  answer = "Good :) \n\0";
                  write(connfd, answer, strlen(answer));
              }
              for (x = 0; x < 10; x++)
                  nick[x] = 0;
            }

            /*Deolve hora, estamos usando srtncmp pois existe um \n 
            no final de recvline                                 */
            if (strncmp(recvline, "MACHORA", 7) == 0) {
               tm_p = localtime( &hora );
               sprintf(hour, "%d:%d:%d--%s\n",tm_p->tm_hour, tm_p->tm_min, tm_p->tm_sec,tm_p->tm_zone);
               write(connfd, hour, strlen(hour));
               for (x = 0; x < 14; x++)
                 hour[x] = 0;
            }

            /*Devolve a data do servidor no formato desejado*/
            if (strncmp(recvline, "MACDATA", 7) == 0) {
               strncat(data, __DATE__ +4, 2);
               data[2] = '/';
               strncat(data, __DATE__ , 3);
               data[5] = '/';
               strncat(data, __DATE__ +7, 4);
               strncat(data, "\n", 1);
               write(connfd, data, strlen(data));
               for (x = 0; x < 13; x++)
                 data[x] = 0;
            }

            /*Devolve a temperatura da cidade de São Paulo de acordo o site http://www.estacao.iag.usp.br/ */

            if (strncmp(recvline, "MACTEMPERATURA", 14) == 0) {
               getTemperatura(temp);
               sprintf(tempsp, "%s-http://www.estacao.iag.usp.br/\n",temp);
               write(connfd, tempsp, strlen(tempsp));
               for (x = 0; x < strlen(tempsp); x++)
                 data[x] = 0;
            }

            if (strncmp(recvline, "QUIT", 4) == 0) {
                break;
            }

         }
         /* ========================================================= */
         /* ========================================================= */
         /*                         EP1 FIM                           */
         /* ========================================================= */
         /* ========================================================= */

         /* Ap�s ter feito toda a troca de informa��o com o cliente,
          * pode finalizar o processo filho */
         printf("[Uma conexao fechada]\n");
         exit(0);
      }
      /**** PROCESSO PAI ****/
      /* Se for o pai, a �nica coisa a ser feita � fechar o socket
       * connfd (ele � o socket do cliente espec�fico que ser� tratado
       * pelo processo filho) */
		close(connfd);
	}
  fclose(file);
  //fclose(client);
	exit(0);
}

void getTemperatura(char temp[]) {
    char http[28];
    int x;
    for (x = 0; x < 7; x++) temp[x] = 0;
    FILE *file;
    system("GET http://www.estacao.iag.usp.br/ | grep \"temp\" > temp.txt");
    file = fopen("temp.txt","r");
    fgets(http,28,file);
    strncat(temp, http + 19, 7);
    fclose(file);
    system("rm temp.txt");
}

void insert_client(char *nick) {
    FILE *f = fopen("nicks.txt", "a");
    fputs(nick, f);
    fputs("\0", f);
    fclose(f);
}

int check_nick(char *nick) {
   FILE *f;
   int i;
   char comp[10];
   f = fopen("nicks.txt", "r");

   for (i = 0; i < 10; i++)
       comp[i] = 0;
   if (f != NULL) {   

      while (fgets(comp, 10, f) != NULL) {
         if (strncmp(comp, nick, 10) == 0) {
             fclose(f);
             return 1;
          }
      }
  }
   fclose(f);
   return 0;
}

void imprime_cliente(node *p) {
  node *aux;
  for (aux = p->prox; aux != NULL; aux = aux->prox) {
    printf("%s\n",aux->nick);
  }
}

void update_client(node *p, FILE *client) {

}

void get_nick(char *line, char nick[]) {
  int x;
  int y = 0;
  for (x = 5; line[x] != '\n';x++) {
     nick[y] = line[x];
      y++;
  }  
  nick[y + 1] = 0;
}