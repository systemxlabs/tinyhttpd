//
// Created by Linwei Zhang on 2022/4/1.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "tinyhttpd.h"

int main() {
    // TODO 重构成读取配置文件
    int server_sockfd = server_start(8888);
    while (1) {
        int client_sockfd = accept_conn(server_sockfd);
        pthread_t new_thread;
        if (pthread_create(&new_thread, NULL, (void *)handle_conn, (void *) (intptr_t) client_sockfd) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }
    server_stop(server_sockfd);
    return 0;
}

int server_start(int port) {
    printf("Server starting on port %d ...\n", port);
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("create socket failed.\n");
        exit(1);
    }
    // 2. 绑定端口
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("server bind failed.\n");
        exit(1);
    }
    // 3. 监听
    if (listen(sockfd, 20) < 0) {
        perror("server listen failed.\n");
        exit(1);
    }
    return sockfd;
}

int accept_conn(int server_sockfd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);
    if (client_sockfd < 0) {
        perror("accept conn failed.\n");
        exit(1);
    }
    printf("Accept a new connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return client_sockfd;
}

int handle_conn(void *client_sockfd_ptr) {
    int client_sockfd = (int) (intptr_t) client_sockfd_ptr;
    printf("Thread %d handling connection ...\n", (int) pthread_self());
    // 解析请求
    struct http_request_t request;
    // 读取请求
    char buf[1024];
    ssize_t n = recv(client_sockfd, buf, sizeof(buf), 0);
    if (n < 0) {
        perror("recv failed.\n");
        exit(1);
    }
    buf[n] = '\0'; // 字符串结束符
    printf("request: %s\n", buf);

    // 返回响应
    char response[1024] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello World!</h1>";
    if (send(client_sockfd, response, sizeof(response), 0) < 0) {
        perror("send failed.\n");
        exit(1);
    }
    printf("send response: %s\n", response);
    close(client_sockfd);
    return 0;
}

void server_stop(int sockfd) {
    printf("Server stopping...\n");
    close(sockfd);
}