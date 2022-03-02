#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "masterlib.h"


int main()
{
    // apre il thread che supporta la comunicazione UDP
    thread_UDPserver();

    // crea un server TCP ricorsivo (gestisce molteplici e simultanei client)
    TCPserver();

    return EXIT_SUCCESS;
}
