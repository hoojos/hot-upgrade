#include "server.h"

int create_listen_tcp_socket(char *addr, uint16_t port) {
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

    if (listen(server_fd, 10240) == -1) {
        perror("listen failed");
        return -1;
    }
        
    printf("Server is running on  %s:%d\n", addr, port);
    return server_fd;
}

server_t *create_server() {
    int      server_fd, epoll_fd;
    server_t *srv;
    struct   epoll_event ev;

    // 创建监听的 socket
    server_fd = create_listen_tcp_socket("0.0.0.0", PORT);

    // 创建 epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return NULL;
    }

    // add server fd to epoll
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd failed");
        return NULL;
    }

    srv = malloc(sizeof(server_t));
    srv->listen_fd = server_fd;
    srv->epoll_fd = epoll_fd;
    return srv;
}

int event_loop(server_t *srv) {
    struct epoll_event events[MAX_EVENTS];

    for (; ;) {
        // printf("event loop\n");
        int nfds = epoll_wait(srv->epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait failed");
            return -1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srv->listen_fd) {
                handle_accept(srv);
            } else {
                handle_client_request(srv, events[i]);
            }
        }
    }
}

int handle_accept(server_t *srv) {
    struct epoll_event ev;
    struct sockaddr_in client_address;
    socklen_t          client_address_len = sizeof(client_address);

    int new_fd = accept(srv->listen_fd, (struct sockaddr *)&client_address, &client_address_len);
    if (new_fd == -1) {
        perror("accept failed");
        return -1;
    }

    // Non-blocking mode
    if (fcntl(new_fd, F_SETFL, fcntl(new_fd, F_GETFL) | O_NONBLOCK) == -1) {
        perror("fcntl failed");
        close(new_fd);
        return -1;
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = new_fd;

    if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
        perror("epoll_ctl: new_fd failed");
        close(new_fd);
        return -1;
    }

    return 0;
}

int handle_client_request(server_t *srv, struct epoll_event ev) {
    char buffer[1024] = {0};

    int ret = read(ev.data.fd, buffer, sizeof(buffer));
    if (ret == -1) {
        // 错误码：需要重试
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        close(ev.data.fd);
        epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev);
    } else if (ret == 0) {
        // 客户端关闭连接
        close(ev.data.fd);
        epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev);
    } else {
        // 正常返回
        // printf("Received message from %d: %s\n", ev.data.fd, buffer);
        char *message = "HTTP/1.1 200 OK\nContent-Length:2\n\nOK\n";
        write(ev.data.fd, message, strlen(message));
    }
}

void destroy_server(server_t *srv) {
    close(srv->listen_fd);
    close(srv->epoll_fd);
    free(srv);
}

