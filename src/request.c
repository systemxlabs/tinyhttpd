//
// Created by Linwei Zhang on 2022/4/15.
//

#include "tinyhttpd.h"

struct http_request_t *parse_request(int client_sockfd) {
    // TODO
    struct http_request_t *request = (struct http_request_t *) malloc(sizeof(struct http_request_t));

    request->method = HTTP_GET;
    request->path = "/index.html";
    request->query_string = "name=Tom";
    request->body = NULL;
    return request;
}

bool is_static_request(struct http_request_t *request) {
    if (request == NULL || request->path == NULL) {
        return false;
    }
    if (strcasecmp(request->method, HTTP_GET) == 0 &&
            (str_end_with(request->path, ".html")
                || str_end_with(request->path, ".css")
                || str_end_with(request->path, ".js"))) {
        return true;
    }
    return false;
}