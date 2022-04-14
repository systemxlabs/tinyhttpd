//
// Created by Linwei Zhang on 2022/4/14.
//

#include <stdio.h>
#include <string.h>
#include "tinyhttpd.h"

struct http_method_t *http_method_by_name(char *method_name) {
    struct http_method_t *method = NULL;
    int method_num = sizeof (http_methods) / sizeof (struct http_method_t);
    for (int i = 0; i < method_num; i++) {
        if (strcmp(method_name, http_methods[i].method_name) == 0) {
            method = &http_methods[i];
            break;
        }
    }
    return method;
}
struct http_method_t *http_method_by_id(enum http_method_e method_id) {
    struct http_method_t *method = NULL;
    int method_num = sizeof (http_methods) / sizeof (struct http_method_t);
    for (int i = 0; i < method_num; i++) {
        if (method_id == http_methods[i].method_id) {
            method = &http_methods[i];
            break;
        }
    }
    return method;
}
