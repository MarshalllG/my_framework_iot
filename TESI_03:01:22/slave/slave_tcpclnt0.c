#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define IP "192.168.1.10"
#define PORT 10000
#define SIZE 1024
#define CLIENT_COMMAND "2"

int main(int argc, char *argv[])
{
  int sk;
  struct sockaddr_in srv_addr;
  socklen_t addr_size;
  char buffer[SIZE];

  sk = socket(AF_INET, SOCK_STREAM, 0);
  if (sk < 0){
    perror("socket error\n");
    exit(1);
  }
  printf("TCP server socket created\n");

  memset(&srv_addr, '\0', sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = PORT;
  srv_addr.sin_addr.s_addr = inet_addr(IP);

  connect(sk, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
  printf("connected to the server with ip %s\n", IP);

  bzero(buffer, SIZE);
  recv(sk, buffer, sizeof(buffer), 0);
  printf("server: %s\n", buffer);

  bzero(buffer, SIZE);
  strcpy(buffer, CLIENT_COMMAND);
  printf("client: %s\n", buffer);
  send(sk, buffer, strlen(buffer), 0);

  bzero(buffer, SIZE);
  recv(sk, buffer, sizeof(buffer), 0);
  printf("server: %s\n", buffer);


  close(sk);
  printf("disconnected from the server\n");

  return EXIT_SUCCESS;
}

