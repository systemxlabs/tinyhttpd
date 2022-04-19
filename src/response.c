//
// Created by Linwei Zhang on 2022/4/15.
//

#include "tinyhttpd.h"

char *generate_raw_response(struct http_response_t *response) {
    char raw_response[1024];
    memset(raw_response, 0, sizeof(raw_response));

    // 响应状态行
    strcat(raw_response, response->version);
    strcat(raw_response, " ");
    char status_code[10];
    sprintf(status_code, "%d ", response->status_code);
    strcat(raw_response, status_code);
    strcat(raw_response, response->status_text);
    strcat(raw_response, "\r\n");

    // 响应头Cookie
    if (response->cookie != NULL) {
        strcat(raw_response, "Set-Cookie: ");
        strcat(raw_response, response->cookie);
        strcat(raw_response, "\r\n");
    }

    // 响应头Content-Type
    if (response->content_type != NULL) {
        strcat(raw_response, "Content-Type: ");
        strcat(raw_response, response->content_type);
        strcat(raw_response, "\r\n");
    }

    // 响应头Content-Length
    strcat(raw_response, "Content-Length: ");
    char content_length[10];
    sprintf(content_length, "%d", response->content_length);
    strcat(raw_response, content_length);
    strcat(raw_response, "\r\n");

    // 空行
    strcat(raw_response, "\r\n");

    // 响应体
    strcat(raw_response, response->body);

    return strdup(raw_response);
}

struct http_response_t *build_response_404() {
    struct http_response_t *response = (struct http_response_t *) malloc(sizeof(struct http_response_t));
    response->version = HTTP_VERSION_11;
    response->status_code = 404;
    response->status_text = "Not Found";
    response->content_type = "text/html";

    // TODO 从文件中读取404页面
    char *response_404_html = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
    response->content_length = strlen(response_404_html);
    response->body = response_404_html;
    return response;
}

struct http_response_t *build_response_501() {
    struct http_response_t *response = (struct http_response_t *) malloc(sizeof(struct http_response_t));
    response->version = HTTP_VERSION_11;
    response->status_code = 501;
    response->status_text = "Not Implemented";
    return response;
}

struct http_response_t *build_response_505() {
    struct http_response_t *response = (struct http_response_t *) malloc(sizeof(struct http_response_t));
    response->version = HTTP_VERSION_11;
    response->status_code = 505;
    response->status_text = "HTTP Version Not Supported";
    return response;
}
