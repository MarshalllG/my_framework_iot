#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#define SIZE 50
#define IP_SIZE 20
#define IP "192.168.1.10"
#define UDP_PORT 8000



void handle_error (char *message);
void init_rand();
int rand_gen (int min, int max);



// riceve le struct e il socket, restituisce 1 se il dispositivo è attivo, 0 altrimenti
int main ()
{
   struct sockaddr_in Srv, fromAddr;
   int sk, i;
   int state_info[SIZE];
   char msg_to_send[SIZE];
   char udp_response[SIZE]; 
   unsigned int fromSize;
   int control_info = 1; // inizialmente l'attesa è 1 sec
 
   // SOCKET
   if ((sk = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
      handle_error("error in socket\n");

   // SERVER INFO
   memset (&Srv, 0, sizeof(Srv));
   Srv.sin_family = PF_INET;
   Srv.sin_port = htons (UDP_PORT);
   Srv.sin_addr.s_addr = inet_addr (IP);

   // generazione del vettore di valori simulati
   for (i = 0; i < SIZE; i++)
      state_info[i] = rand_gen (35, 55);
   i = 0;

   // LOOP
   do 
   {
      sleep(control_info);

      // simulazione di un valore casuale, informazione di stato da comunicare al server UDP nel master
      init_rand();
      sprintf (msg_to_send, "%d", state_info[i]);
      strcat(msg_to_send, " gradi °C");

      // invia un messaggio udp
      if (sendto(sk, msg_to_send, sizeof(msg_to_send), 0, (struct sockaddr *) &Srv, sizeof(Srv)) < 0)
      {
         printf("udp message error, can't send buff\n");
         exit (EXIT_FAILURE);
      }
      printf ("msg sent to server: %s\n", msg_to_send);

      // se ricevo risposta restituisco 1
      fromSize = sizeof (fromAddr);
      if (recvfrom(sk, udp_response, SIZE, 0, (struct sockaddr*) &fromAddr, &fromSize) < 0)
         handle_error ("error in recvrom\n");

      if (strlen(udp_response) <= 0)
      {
         printf ("nothing was received\n");
         return 0;
      }
      else 
      {
         // prima separa l'info di controllo nella risposta del server
         control_info = atoi (udp_response);
      }

      printf ("tempo d'attesa ricevuto dal server, attendo %d secondi\n", control_info);
      sleep(control_info);

      i++;

      if (i == SIZE-1)
         i = 0;

   }while(1);

   return EXIT_SUCCESS;
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