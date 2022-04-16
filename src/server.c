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
#include <string.h>
#include "tinyhttpd.h"

int main() {
    // TODO 重构成读取配置文件端口号
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
    printf("Thread %d handling connection %d ...\n", (int) pthread_self(), client_sockfd);
    // 解析请求
    struct http_request_t *request = parse_request(client_sockfd);
    printf("Thread %d parsed request: %s %s\n", (int) pthread_self(), request->method, request->path);

    // 分发请求进行处理
    struct http_response_t *response = process_request(request);
    printf("Thread %d return response: %d %s %s\n", (int) pthread_self(), response->status_code, response->status_text, response->body);

    // 返回响应
    send_response(client_sockfd, response);

    close(client_sockfd);
    return 0;
}

struct http_response_t *process_request(struct http_request_t *request) {
    // TODO
    printf("Thread %d process request: %s %s\n", (int) pthread_self(), request->method, request->path);
    if (is_static_request(request)) {
        printf("Thread %d is static request\n", (int) pthread_self());
        return execute_file(request);
    }

    // TODO 可配置
    bool cgi_on = true;
    if (cgi_on) {
        printf("Thread %d is cgi request\n", (int) pthread_self());
        return execute_cgi(request);
    }
    bool fcgi_on = true;
    if (fcgi_on) {
        printf("Thread %d is fcgi request\n", (int) pthread_self());
        return execute_fcgi(request);
    }
    return NULL;
}

void send_response(int client_sockfd, struct http_response_t *response) {
    // TODO
    char *response_header = "HTTP/1.1 200 OK\r\n";
    if (send(client_sockfd, response_header, sizeof(response_header), 0) < 0) {
        perror("send response failed.\n");
        exit(1);
    }
}

void server_stop(int sockfd) {
    printf("Server stopping...\n");
    close(sockfd);
}