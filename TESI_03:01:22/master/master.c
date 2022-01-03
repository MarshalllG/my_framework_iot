#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "masterlib.h"


int main()
{
    thread_UDPserver();

    TCPserver();

    return EXIT_SUCCESS;
}
