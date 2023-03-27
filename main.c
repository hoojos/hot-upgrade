#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"


int main() {
    server_t *srv = create_server();
    event_loop(srv);
    return 0;
}