//
// Created by Linwei Zhang on 2022/4/16.
//

#include "tinyhttpd.h"

struct http_response_t *execute_file(struct http_request_t *request) {
    printf("execute_file for request %s %s\n", request->method, request->path);
    if (strcmp(request->path, "/") == 0) {
        request->path = "/index.html";
    }
// TODO 可配置静态文件目录
    // 读取文件内容
    char *filename = str_concat("/Users/lewis/Github/tinyhttpd/html", request->path);
    char *content = read_file_as_str(filename);
    if (content == NULL) {
        // 文件不存在，返回404
        return build_response_404();
    }
    // 构建响应
    struct http_response_t *response = (struct http_response_t *)malloc(sizeof(struct http_response_t));
    response->version = HTTP_VERSION_11;
    response->status_code = 200;
    response->status_text = "OK";
    response->content_type = "text/html";
    response->content_length = strlen(content);
    response->body = content;
    return response;
}

