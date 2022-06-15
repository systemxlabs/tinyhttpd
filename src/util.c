//
// Created by Linwei Zhang on 2022/4/14.
//

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "tinyhttpd.h"

bool str_end_with(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) {
        return false;
    }
    for (int i = 0; i < suffix_len; i++) {
        if (str[str_len - suffix_len + i] != suffix[i]) {
            return false;
        }
    }
    return true;
}

bool str_start_with(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);
    if (str_len < prefix_len) {
        return false;
    }
    return memcmp(prefix, str, prefix_len) == 0;
}

char *read_file_as_str(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);  // 将文件指针移动到文件末尾
    long fsize = ftell(fp);  // 获取文件大小
    fseek(fp, 0, SEEK_SET);  // 将文件指针移动到文件开头
    char *string = malloc(fsize + 1);  // 分配内存
    fread(string, fsize, 1, fp);  // 读取文件内容, nitems为要读取的个数, 1为每次读取的大小
    fclose(fp);
    string[fsize] = 0; // 将字符串结束符设置为0
    return string;
}

char *str_concat(const char *str1, const char *str2) {
    if (str1 == NULL || str2 == NULL) {
        return NULL;
    }
    char *str = malloc(strlen(str1) + strlen(str2) + 1);
    sprintf(str, "%s%s", str1, str2);
    return str;
}

char* url_decode(const char *encoded_url)
{
    char *decoded_url = (char *) malloc(strlen(encoded_url) + 1);
    char *dst = decoded_url;
    char a, b;
    while (*encoded_url) {
        if ((*encoded_url == '%') &&
            ((a = encoded_url[1]) && (b = encoded_url[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            encoded_url+=3;
        } else if (*encoded_url == '+') {
            *dst++ = ' ';
            encoded_url++;
        } else {
            *dst++ = *encoded_url++;
        }
    }
    *dst++ = '\0';
    return decoded_url;
}