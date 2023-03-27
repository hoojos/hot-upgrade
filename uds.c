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

int create_listen_unix_domain_socket(char *path, int fd) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_in address = {
        .sin_family = AF_UNIX,
    };

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind failed");
        return -1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        return -1;
    }

    printf("Server is running on  %s\n", path);
    return server_fd;
}

int send_fd(int socket, int fd){
    struct msghdr message;
    struct iovec iov[1];

    // 准备要发送的数据
    /* 设置一个字节的辅助数据，表示发送的是文件描述符 */
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr *cmsg = (struct cmsghdr *)cmsgbuf;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmsg)) = fd;

    iov[0].iov_base = " ";
    iov[0].iov_len = 1;

    // 准备要发送的消息
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = cmsg;
    message.msg_controllen = CMSG_SPACE(sizeof(int));
    message.msg_flags = 0;

    // 使用 sendmsg() 函数发送消息
    return sendmsg(socket, &message, 0);
}

int recv_fd(int socket, int *fd){
    struct msghdr message;
    struct iovec iov[1];

    // 准备要接收的数据
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr *cmsg = (struct cmsghdr *)cmsgbuf;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    iov[0].iov_base = " ";
    iov[0].iov_len = 1;

    // 准备要接收的消息
    memset(&message, 0, sizeof(message));
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = cmsg;
    message.msg_controllen = CMSG_SPACE(sizeof(int));
    message.msg_flags = 0;

    // 使用 recvmsg() 函数接收消息
    int len = recvmsg(socket, &message, 0);
    if (len < 0) {
        return -1;
    }

    // 获取文件描述符
    *fd = *((int *) CMSG_DATA(cmsg));
    return 0;
}