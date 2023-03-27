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

#define MAX_EVENTS 1024
#define PORT 8080

int create_listen_socket(char *addr, uint16_t port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == -1) {
        perror("socket failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        return -1;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr(addr),
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind failed");
        return -1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        return -1;
    }
        
    printf("Server is running on  %s:%d\n", addr, port);
    return server_fd;
}

int main() {
    
    int server_fd = create_listen_socket("0.0.0.0", PORT);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return -1;
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd failed");
        return -1;
    }

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait failed");
            return -1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_address;
                socklen_t client_address_len = sizeof(client_address);

                int new_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
                if (new_fd == -1) {
                    perror("accept failed");
                    continue;
                }

                // Non-blocking mode
                if (fcntl(new_fd, F_SETFL, fcntl(new_fd, F_GETFL) | O_NONBLOCK) == -1) {
                    perror("fcntl failed");
                    close(new_fd);
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
                    perror("epoll_ctl: new_fd failed");
                    close(new_fd);
                    continue;
                }

                char ip[16] = {0};
                inet_ntop(AF_INET, &client_address.sin_addr, ip, sizeof(ip));
                // printf("Connection accepted from %s:%d\n", ip, ntohs(client_address.sin_port));
            } else {
                char buffer[1024] = {0};
                int ret = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                if (ret == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    } else {
                        perror("recv failed");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                        close(events[i].data.fd);
                        continue;
                    }
                } else if (ret == 0) {
                    perror("connection closed");
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                    close(events[i].data.fd);
                    continue;
                } else {
                    // printf("Received message from %d: %s\n", events[i].data.fd, buffer);
                    char *message = "HTTP/1.1 200 OK\n\nOK";
                    send(events[i].data.fd, message, strlen(message), 0);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                    close(events[i].data.fd);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    return 0;
}