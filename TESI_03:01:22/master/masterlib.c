#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include "masterlib.h"
#define IP "192.168.1.10"
#define SEMAPH 0


/* struct che raccoglie i vari dispositivi nella rete, i campi sono l'ip e lo stato */
struct device
{
    char ipaddr[IP_SIZE];
    char dev_state[SIZE]; // informazioni di stato
};

/* argomento da passare a pthread_create: struttura che contiene gli indici da usare nel thread */
struct thread_info 
{
    pthread_t thread_id;        /* ID del thread restituito da pthread_create() */
};

// dichiarazione di variabili globali
struct device d[MAX_CLIENTS]; // vettore di struct device

/*********************************************************************************
 *********************************************************************************
 FUNZIONI PER LE COMUNICAZIONI IN TCP
 *********************************************************************************
 ********************************************************************************/


void TCPserver (void)
{
    struct sockaddr_in saddr, caddr; 
    int rc; // per il thread
    int listensk, sk; // uso due socket: uno di ascolto e uno per gestire la connessione
    socklen_t addr_size; // per l'accept
    pid_t pid; // process identifier della funzione fork
    pthread_t thread; // singolo thread per comunicazioni UDP
    char clnt_ip[IP_SIZE]; 



    // SOCKET
    if ((listensk = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        handle_error("error in socket\n");

    // SERVER INFO
    memset(&saddr, '\0', sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = TCP_PORT;
    saddr.sin_addr.s_addr = inet_addr(IP);

    // BIND
    if ((bind(listensk, (struct sockaddr*)&saddr, sizeof(saddr))) < 0)
        handle_error("error in bind\n");
    printf("TCP SERVER: bind to: ip %s port %d\n", IP, TCP_PORT);

    // LISTEN
    if (listen (listensk, QUEUELEN) < 0)
        handle_error("error in listen\n");
    printf("TCP server is listening...\n");


    // LOOP
    do
    {    
        // ACCEPT CLIENT
        addr_size = sizeof(caddr);
        if ((sk = accept(listensk, (struct sockaddr*)&caddr, &addr_size)) < 0)
            handle_error("error in accept\n");

        printf ("-----------------------------\n");    
        printf("TCP connection accepted, client has ip %s port %d\n", inet_ntoa(caddr.sin_addr), caddr.sin_port);
        
 
        // FORK
        if ((pid = fork()) == -1)
        {
            printf ("\nThis is parent: error creating subprocess\n");
            exit (EXIT_FAILURE);
        }

        // CHILD WILL SERVE THE CLIENT (and then, it will terminate)
        if (pid == 0)
        {  
           close (listensk);
           strcpy (clnt_ip, inet_ntoa(caddr.sin_addr));
           serve_client (sk, clnt_ip);
           close (sk);
           printf ("client disconnected\n");
           printf ("-----------------------------\n");    
           exit(0); // se non chiudo il processo mi da' errore sull'accept
        }
        

        close (sk);
    
    }while (1);

    return;
}
    


void serve_client (int csk, char clnt_ip[])
{
   char buffer[SIZE];
   FILE *fp;
   int n_ip;
   int i = 0;

   /* se non presente nel file, aggiunge il client attualmente connesso alla lista */
   if (search_static_ip (fp, FILENAME, clnt_ip) == 0)
      add_device (fp, FILENAME, clnt_ip);
 
   //restituisce il numero degli ip registrati
   n_ip = load_devices (fp, FILENAME, MAX_CLIENTS); 

   // dialoga in TCP con il client
   bzero(buffer, SIZE);
   strcpy(buffer, "client connesso: di quale dispositivo devo verificare la temperatura?\nInserire numero dello slave\n");
   send(csk, buffer, strlen(buffer), 0);

   bzero(buffer, SIZE);
   recv(csk, buffer, sizeof(buffer), 0);
   if (atoi(buffer) >= 0)
   {    
      i = atoi(buffer);
      printf("il client %s vuole conoscere la temperatura del device con ip %s\n", clnt_ip, d[i].ipaddr);
   }

   if (i > sizeof(d))
   {
      bzero(buffer, SIZE);
      strcpy(buffer, "errore, dispositivo selezionato non esistente\n");
      send(csk, buffer, strlen(buffer), 0);
      perror ("errore, dispositivo selezionato non esistente\n");
   }
    
   if (atoi(buffer) >= 0)
   {
      bzero(buffer, SIZE);
      strcpy(buffer, d[i].dev_state);
      send(csk, buffer, strlen(buffer), 0);
      printf ("messaggio spedito al client TCP: %s\n", buffer);
   }
   else
   {
      bzero (buffer, SIZE);
      printf ("Option not avaiable, closing connection...\n");
      strcpy (buffer, "Option not avaiable, closing connection...\n");
      send (csk, buffer, strlen(buffer), 0);
   }
     
   return;
}



/*********************************************************************************
 *********************************************************************************
 FUNZIONI PER LE COMUNICAZIONI IN UDP
 *********************************************************************************
 ********************************************************************************/


/* crea un thread per la comunicazione UDP */
void thread_UDPserver (void)
{
   pthread_t thread;
   int i, rc, res;


   // allocazione di memoria per gli argomenti della pthread_create
   struct thread_info *tinfo = calloc(MAX_CLIENTS, sizeof(*tinfo));
   if (tinfo == NULL)
      handle_error("calloc");

   // 1° argomento: scrivo l'id del thread in un campo della struct
   // 3° argomento: funzione che contiene il codice in base a cui avviene l'elaborazione
   // 4° argomento: passa una struttura dati come argomento per la checkstate
   rc = pthread_create (&tinfo -> thread_id, NULL, udp_polling, &tinfo);

   printf("created thread, id %ld\n", (long int) tinfo -> thread_id);

   if (rc)
   { 
      printf("ERROR: return code from pthread_create() is %d\n", rc);
      exit (EXIT_FAILURE);
   }

   return;
}



// esegue una scansione periodica degli stati dei dispositivi registrati
void *udp_polling (void *arg)
{
   struct thread_info *tinfo = arg;
   int sk, received_msg_size, wait_time;
   char buff[UDP_BUFF];
   struct sockaddr_in Srv, Clnt;
   unsigned int ClntAddrLen;    

   // SOCKET
   if((sk = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
   {
      handle_error ("error in socket\n");
      exit (EXIT_FAILURE);
   }

   // INFO SERVER UDP
   memset (&Srv, 0, sizeof (Srv));
   Srv.sin_family = AF_INET;
   Srv.sin_addr.s_addr = inet_addr (IP);
   Srv.sin_port = htons (UDP_PORT);

   // BIND
   if (bind (sk, (struct sockaddr *) &Srv, sizeof (Srv)) < 0)
   {
      handle_error ("error in bind\n");
      exit (EXIT_FAILURE);
   }
   printf ("UDP SERVER: bind successfull to IP: %s, port: %d\n", IP, UDP_PORT);    

   // NON-BLOCKING MODE
   udp_set_non_blocking_mode(sk);
   
   ClntAddrLen = sizeof (Clnt);

   printf ("UDP polling started now...\n");

   do
   {
      // preparo il buffer
      memset (buff, 0, UDP_BUFF);
            
      do
      {
         if ((received_msg_size = recvfrom(sk, buff, UDP_BUFF, 0, (struct sockaddr*) &Clnt, &ClntAddrLen) <= 0) || strlen (buff) == 0)
         {
            sleep (5);
         }
      } while (received_msg_size <= 0 || strlen (buff) == 0);
      // qualcosa è stato ricevuto
         
      printf("Received UDP message: %s from port %d, ip address %s\n", buff, ntohs(Clnt.sin_port), inet_ntoa(Clnt.sin_addr));

      // aggiorno le informazioni di stato
      update_state (inet_ntoa(Clnt.sin_addr), buff);

      // preparo il buffer
      memset (buff, 0, UDP_BUFF);

      // generazione del tempo d'attesa per il client
      wait_time = 45;
      sprintf (buff, "%d", wait_time);
      printf ("generato tempo d'attesa per il client udp con ip %s: %d secondi\n", inet_ntoa(Clnt.sin_addr), wait_time);

      // comunico al client l'attesa prima di rispedire
      if (sendto(sk, buff, sizeof(buff), 0, (struct sockaddr *) &Clnt, ClntAddrLen) < 0)
      {
         printf("udp message error, can't send buff\n");
         exit (EXIT_FAILURE);
      }
      printf ("prossima informazione di stato dal client udp con ip %s: tra %s secondi\n*****************************\n", inet_ntoa(Clnt.sin_addr), buff);
         
   } while (1);
       
   // close socket
   close(sk);
    
   pthread_exit (NULL);
}



void udp_set_non_blocking_mode (int sk)
{
   int flags;
   
   flags = fcntl (sk, F_GETFL, 0);
   fcntl(sk, F_SETFL, flags | O_NONBLOCK);
   
   return;
}



///////////////////////////////////////////////
//FUNZIONI DI UTILITY//////////////////////////
///////////////////////////////////////////////



void add_device (FILE *fp, char file_name[], char ip[])
{
   // apertura del file in modalità "append"
   if ((fp = fopen(FILENAME, "a")) == NULL)
   {
      fprintf (stderr, "not able to open file %s\n", file_name);
      exit (EXIT_FAILURE);
   }

   // aggiunge l'ip
   fprintf (fp, "%s\n", ip);
   printf ("adding ip %s\n", ip);
   fclose (fp);

   return;
}



// carica un vettore di struct device leggendo da file
int load_devices (FILE *fp, char file_name[], int max_devices)
{
   int i = 0;
   char tmp_string[IP_SIZE][MAX_CLIENTS];
   
   // apertura del file in modalità di lettura
   if ((fp = fopen (file_name, "r")) == NULL)
   {
      printf ("cannot open file %s, error in file opening\n", file_name);
      exit (EXIT_FAILURE);
   }

   while ((i < max_devices) && (fgets (tmp_string[i], IP_SIZE, fp) != NULL))
   {
      if (strlen(tmp_string[i]) > 2)
         strcpy (d[i].ipaddr, tmp_string[i]);

      i++; 
   }

   // chiude il file
   fclose(fp);
   
   return i;
} 



// stampa il vettore di struct
void print_devices (int n_devices)
{
   int i;
   
   for (i = 1; i <= n_devices; i++)
   {
      printf ("d[%d].ipaddr = %s\n", i, d[i].ipaddr);
      printf ("d[%d].dev_state = %s\n", i, d[i].dev_state);
   }
   
   return;
}



// cerca un ip nel file: se lo trova restituisce l'indice per la struct device, altrimenti 0
int search_static_ip (FILE *fp, char file_name[], char clnt_ip[])
{
   int i;
   char tmp[SIZE];
    
   // apertura del file in modalità di lettura
   if ((fp = fopen(FILENAME, "r")) == NULL)
   {
      fprintf (stderr, "not able to open file %s\n", file_name);
      exit (EXIT_FAILURE);
   }
   
   i = 0;
   while (fscanf (fp, "%s", tmp) != EOF)
   {
      if (strcmp (clnt_ip, tmp) == 0)
         return i;

      i++;
   }

   // chiude il file
   fclose(fp);

   return 0;
}



int update_state (char ip[], char new_state[])
{
   char temp_i[SIZE];
   FILE *fp;
   int index;

   // copio nel vettore di struct l'informazione di stato spedita dal client udp
   if ((index = search_static_ip (fp, FILENAME, ip)) >= 0)
   {
      // aggiorno lo stato
      strcpy(d[index].dev_state, new_state);

      printf ("-> modifica stato client %s: %s\n", ip, d[index].dev_state);
      return 1; // restituisco modifica avvenuta con successo
   }
   else
   {
      printf ("client non registrato, non è stata aggiunta alcuna informazione di stato\n");
   }

   return 0; // modifica non avvenuta
}



void init_rand()
{
   time_t t;
   time (&t);

   srand (t);

   return;
}



int rand_gen (int min, int max)
{
   double k;
   int rand_n;
   
   if (min < max)
      k = (max - min) / (double) RAND_MAX;
   else  /* forgive them... */
      k = (min - max) / (double) RAND_MAX;
   
   rand_n = rand();

   if (min < max)
      return lround (rand_n * k + min);
   else  /* forgive them... */
      return lround (rand_n * k + max);
}   
   


void handle_error (char *message)
{
   printf ("fatal error: %s\n", message);
   exit (EXIT_FAILURE);
}


