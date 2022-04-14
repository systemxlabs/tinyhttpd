//
// Created by Linwei Zhang on 2022/4/1.
//

#ifndef TINYHTTPD_TINYHTTPD_H
#define TINYHTTPD_TINYHTTPD_H

#endif //TINYHTTPD_TINYHTTPD_H

enum http_method_e {
    GET,
    POST
};
struct http_method_t {
    enum http_method_e method_id;
    const char *method_name;
};
static struct http_method_t http_methods[] = {
        {GET, "GET"},
        {POST, "POST"},
};

struct http_request_t {
    struct http_method_t *method;
    const char *path;
    const char *query_string;
    const char *body;
    const char *headers;
};
struct http_response_t {
    int status_code;
    const char *status_text;
    const char *body;
    const char *headers;
};


// 服务启动，返回socket描述符
int server_start(int port);

// 接收客户端连接
int accept_conn(int server_sockfd);

// 处理客户端连接
int handle_conn(void *client_sockfd_ptr);

// 处理请求
int handle_request(struct http_request_t *request, struct http_response_t *response);

// 服务停止
void server_stop(int sockfd);
void send_response(int client_sock, char *response);
void send_error(int client_sock, int error_code);

struct http_method_t *http_method_by_name(char *method_name);
struct http_method_t *http_method_by_id(enum http_method_e method_id);