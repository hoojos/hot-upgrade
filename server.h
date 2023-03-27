#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORT 8080
#define PATH "/tmp/server.sock"

#define MAX_EVENTS 1024

typedef struct {
    int listen_fd;
    int epoll_fd;
} server_t;

server_t *create_server();
int event_loop(server_t *srv);
void destroy_server(server_t *srv);