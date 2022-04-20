//
// Created by Linwei Zhang on 2022/4/1.
//

#ifndef TINYHTTPD_TINYHTTPD_H
#define TINYHTTPD_TINYHTTPD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define STDIN_FILENO    0       /* Standard input.  */
#define STDOUT_FILENO   1       /* Standard output.  */
#define STDERR_FILENO   2       /* Standard error output.  */

#define HTTP_VERSION_11 "HTTP/1.1"

#define HTTP_GET "GET"
#define HTTP_POST "POST"

struct http_request_t {
    char *method;            // http方法
    char *path;              // 请求路径
    char *query_string;      // 查询字符串
    char *fragment;          // 锚点
    char *scheme;            // http协议
    char *version;           // http版本
    char *host;              // 请求的host
    char *content_type;      // 请求的content_type
    int content_length;      // 请求的content_length
    char *cookie;            // 请求的cookie
    char *connection;        // 请求的connection
    char *body;              // 请求体
};
struct http_response_t {
    char *version;           // http版本
    int status_code;               // http状态码
    char *status_text;       // http状态码描述
    char *cookie;            // 响应的cookie
    char *content_type;      // 响应的content_type
    int content_length;      // 响应的content_length
    char *body;              // 响应体
    char *raw_response;      // 原始响应
};


/****************************
 * server.c
 ***************************/
// 服务启动，返回socket描述符
int server_start(int port);
// 服务停止
void server_stop(int sockfd);
// 接收客户端连接
int accept_conn(int server_sockfd);
// 处理客户端连接
int handle_conn(void *client_sockfd_ptr);
// 处理请求
struct http_response_t *process_request(struct http_request_t *request);
// 往客户端发送响应
void send_response(int client_sockfd, struct http_response_t *response);


/****************************
 * request.c
 ***************************/
// 解析请求
struct http_request_t *parse_request(char *raw_request);
// 验证请求
struct http_response_t *validate_request(struct http_request_t *request);
// 判断是否为静态请求
bool is_static_request(struct http_request_t *request);


/****************************
 * response.c
 ***************************/
// 生成response字符串
char *generate_raw_response(struct http_response_t *response);
// 构造只含原始响应数据的response
struct http_response_t *build_raw_response(char *raw_response);
// 构造404 Not Found响应
struct http_response_t *build_response_404();
// 构造500 Internal Server Error响应
struct http_response_t *build_response_500();
// 构造501 Not Implemented响应
struct http_response_t *build_response_501();
// 构造505 HTTP Version not supported响应
struct http_response_t *build_response_505();


/****************************
 * serve_file.c
 ***************************/
// 处理静态资源请求
struct http_response_t *execute_file(struct http_request_t *request);


/****************************
 * serve_cgi.c
 ***************************/
// 处理CGI动态请求
struct http_response_t *execute_cgi(struct http_request_t *request);
// 执行Python CGI脚本
struct http_response_t *execute_cgi_python(struct http_request_t *request);


/****************************
 * serve_fcgi.c
 ***************************/
// 处理FCGI动态请求
struct http_response_t *execute_fcgi(struct http_request_t *request);


/****************************
 * util.c
 ***************************/
bool str_end_with(const char *str, const char *suffix);
char *read_file_as_str(const char *filename);
char *str_concat(const char *str1, const char *str2);

#endif //TINYHTTPD_TINYHTTPD_H