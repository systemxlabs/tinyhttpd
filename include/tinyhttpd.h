//
// Created by Linwei Zhang on 2022/4/1.
//

#ifndef TINYHTTPD_TINYHTTPD_H
#define TINYHTTPD_TINYHTTPD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_GET "GET"
#define HTTP_POST "POST"

struct http_request_t {
    const char *method;            // http方法
    const char *path;              // 请求路径
    const char *query_string;      // 查询字符串
//    const char *fragment;          // 锚点
//    const char *scheme;            // http协议
//    const char *version;           // http版本
//    const char *host;              // 请求的host
//    const char *content_type;      // 请求的content_type
//    const char *content_length;    // 请求的content_length
//    const char *cookie;            // 请求的cookie
    const char *body;              // 请求体
};
struct http_response_t {
    const char *version;           // http版本
    int status_code;               // http状态码
    const char *status_text;       // http状态码描述
    const char *content_type;      // 响应的content_type
    int content_length;     // 响应的content_length
    const char *cookie;            // 响应的cookie
    const char *body;              // 响应体
};


// 服务启动，返回socket描述符
int server_start(int port);
// 服务停止
void server_stop(int sockfd);

// 接收客户端连接
int accept_conn(int server_sockfd);
// 处理客户端连接
int handle_conn(void *client_sockfd_ptr);

// 解析请求
struct http_request_t *parse_request(int client_sockfd);

// 验证请求
struct http_response_t *validate_request(struct http_request_t *request);

// 处理请求
struct http_response_t *process_request(struct http_request_t *request);

// 判断是否为静态请求
bool is_static_request(struct http_request_t *request);

// 构造响应
struct http_response_t *build_response_501();
struct http_response_t *build_response_404();

// 往客户端发送响应
void send_response(int client_sockfd, struct http_response_t *response);

// 处理静态资源请求
struct http_response_t *execute_file(struct http_request_t *request);
// 处理CGI动态请求
struct http_response_t *execute_cgi(struct http_request_t *request);
// 处理FCGI动态请求
struct http_response_t *execute_fcgi(struct http_request_t *request);

bool str_end_with(const char *str, const char *suffix);
char *read_file_as_str(const char *filename);
char *str_concat(const char *str1, const char *str2);

#endif //TINYHTTPD_TINYHTTPD_H