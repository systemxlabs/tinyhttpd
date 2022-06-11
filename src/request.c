//
// Created by Linwei Zhang on 2022/4/15.
//

#include "tinyhttpd.h"

struct http_request_t *parse_request(char *raw_request) {
    struct http_request_t *request = (struct http_request_t *) malloc(sizeof(struct http_request_t));
    memset(request, 0, sizeof(struct http_request_t));

    // 解析http方法
    size_t method_len = strcspn(raw_request, " ");
    request->method = (char *) malloc(method_len + 1);
    memcpy(request->method, raw_request, method_len);
    request->method[method_len] = '\0';
    raw_request += method_len + 1; // 跳过空格

    // 解析url
    size_t url_len = strcspn(raw_request, " ");
    char *url = (char *) malloc(url_len + 1);
    memcpy(url, raw_request, url_len);
    url[url_len] = '\0';
    raw_request += url_len + 1; // 跳过空格
    // 解析url中的path
    size_t path_len = strcspn(url, "?");
    char *path = (char *) malloc(path_len + 1);
    memcpy(path, url, path_len);
    path[path_len] = '\0';
    request->path = path;
    url += path_len + 1; // 跳过空格
    // 解析url中的query_string
    size_t query_string_len = strlen(url);
    char *query_string = (char *) malloc(query_string_len + 1);
    memcpy(query_string, url, query_string_len);
    query_string[query_string_len] = '\0';
    request->query_string = query_string;

    // 解析http版本
    size_t version_len = strcspn(raw_request, "\r\n");
    request->version = (char *) malloc(version_len + 1);
    memcpy(request->version, raw_request, version_len);
    request->version[version_len] = '\0';
    raw_request += version_len + 2; // 跳过\r\n

    // 解析header
    while (raw_request[0] != '\r' || raw_request[1] != '\n') {
        size_t key_len = strcspn(raw_request, ":");
        char *key = (char *) malloc(key_len + 1);
        memcpy(key, raw_request, key_len);
        key[key_len] = '\0';
        raw_request += key_len + 2; // 跳过: 和空格

        size_t value_len = strcspn(raw_request, "\r\n");
        char *value = (char *) malloc(value_len + 1);
        memcpy(value, raw_request, value_len);
        value[value_len] = '\0';
        raw_request += value_len + 2; // 跳过\r\n

        if (strcmp(key, "Content-Length") == 0) {
            request->content_length = atoi(value);
        }
        if (strcmp(key, "Host") == 0) {
            request->host = value;
        }
        if (strcmp(key, "Connection") == 0) {
            request->connection = value;
        }
        if (strcmp(key, "Cookie") == 0) {
            request->cookie = value;
        }
    }

    raw_request += 2; // 跳过\r\n 空行

    // 解析body
    size_t body_len = strlen(raw_request);
    request->body = (char *) malloc(body_len + 1);
    memcpy(request->body, raw_request, body_len);
    request->body[body_len] = '\0';

    return request;
}

bool is_static_request(struct http_request_t *request) {
    if (request == NULL || request->path == NULL) {
        return false;
    }
    // 如果path为"/"，则认为是静态请求，自动返回/index.html
    if (strcasecmp(request->method, HTTP_GET) == 0 && strcmp(request->path, "/") == 0) {
        return true;
    }
    if (strcasecmp(request->method, HTTP_GET) == 0 &&
            (str_end_with(request->path, ".html")
                || str_end_with(request->path, ".css")
                || str_end_with(request->path, ".js")
                || str_end_with(request->path, ".ico"))) {
        return true;
    }
    return false;
}

bool is_cgi_request(struct http_request_t *request) {
    if (request == NULL || request->path == NULL) {
        return false;
    }
    // 如果path起始为"/cgi-bin"，则认为是cgi请求
    return str_start_with(request->path, "/cgi-bin");
}

bool is_fcgi_request(struct http_request_t *request) {
    if (request == NULL || request->path == NULL) {
        return false;
    }
    // 如果path起始为"/fcgi-bin"，则认为是fcgi请求
    return str_start_with(request->path, "/fcgi-bin");
}

bool is_proxy_request(struct http_request_t *request) {
    if (request == NULL || request->path == NULL) {
        return false;
    }
    // 如果path起始为"/proxy"，则认为是proxy请求
    return str_start_with(request->path, "/proxy");
}

struct http_response_t *validate_request(struct http_request_t *request) {
    if (strcasecmp(request->method, HTTP_GET) != 0
            && strcasecmp(request->method, HTTP_POST) != 0) {
        return build_response_501();
    }
    if (strcasecmp(request->version, HTTP_VERSION_11) != 0) {
        return build_response_505();
    }
    return NULL;
}