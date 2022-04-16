//
// Created by Linwei Zhang on 2022/4/15.
//

#include "tinyhttpd.h"

struct http_response_t *build_response_501() {
    // TODO
}

struct http_response_t *build_response_404() {
    struct http_response_t *response = (struct http_response_t *) malloc(sizeof(struct http_response_t));
    response->version = "HTTP/1.1";
    response->status_code = 404;
    response->status_text = "Not Found";
    response->content_type = "text/html";

    // TODO 从文件中读取404页面
    char *response_404_html = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
    response->content_length = strlen(response_404_html);
    response->body = response_404_html;
    return response;
}