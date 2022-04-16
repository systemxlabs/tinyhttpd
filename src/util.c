//
// Created by Linwei Zhang on 2022/4/14.
//

#include <stdbool.h>
#include <string.h>
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