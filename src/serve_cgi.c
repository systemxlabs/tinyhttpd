//
// Created by Linwei Zhang on 2022/4/16.
//

#include "tinyhttpd.h"

#define MAXLINE 4096

struct http_response_t *execute_cgi(struct http_request_t *request) {
    if (str_end_with(request->path, ".py")) {
        return execute_cgi_python(request);
    }
    return build_response_500();
}
struct http_response_t *execute_cgi_python(struct http_request_t *request) {
    int n, parent_child_pipe[2], child_parent_pipe[2];
    pid_t pid;
    char raw_response[MAXLINE];
    int rv;  // 子进程返回值

    // 创建管道
    if (pipe(parent_child_pipe) < 0 || pipe(child_parent_pipe) < 0) {
        puts("Error creating pipes...\n");
        return build_response_500();
    }

    // fork子进程
    if ( (pid = fork()) < 0) {  // 创建子进程
        puts("Error forking...\n");
        return build_response_500();
    }

    if (pid == 0) {
        // 子进程
        close(parent_child_pipe[1]);  // 关闭父进程到子进程的管道写端
        close(child_parent_pipe[0]);  // 关闭子进程到父进程的管道读端

        // 将父进程到子进程的管道读端重定向到标准输入
        dup2(parent_child_pipe[0], STDIN_FILENO);
        // 将子进程到父进程的管道写端重定向到标准输出
        dup2(child_parent_pipe[1], STDOUT_FILENO);

        // 设置环境变量
        char method_env[255];
        char query_string_env[255];
        char content_type_env[255];
        char content_length_env[255];
        sprintf(method_env, "REQUEST_METHOD=%s", request->method);
        sprintf(query_string_env, "QUERY_STRING=%s", request->query_string);
        sprintf(content_type_env, "CONTENT_TYPE=%s", request->content_type);
        sprintf(content_length_env, "CONTENT_LENGTH=%d", request->content_length);
        putenv(method_env);
        putenv(query_string_env);
        putenv(content_type_env);
        putenv(content_length_env);

        // 执行cgi程序
        char command[255];
        // 拼接成 ./cgi-bin/student.py（cgi脚本会被编译进build目录）
        sprintf(command, ".%s", request->path);
        if (execl(command, command, (char *) 0) < 0) {
            puts(generate_raw_response(build_response_500()));
        }
    } else {
        // 父进程
        close(parent_child_pipe[0]);  // 关闭父进程到子进程的管道读端
        close(child_parent_pipe[1]);  // 关闭子进程到父进程的管道写端

        // 将body数据写入管道
        if (request->content_length > 0) {
            write(parent_child_pipe[1], request->body, request->content_length);
        }

        // 关闭管道会发生EOF https://stackoverflow.com/questions/22032120/closing-pipe-file-descriptor-in-c
        close(parent_child_pipe[1]);  // 关闭父进程到子进程的管道写端

        // TODO 可以等待超时，超时则kill子进程并返回500
        wait(&rv);  // 等待子进程结束，返回子进程的退出状态

        // 读取CGI脚本的输出
        // TODO 优化读取
        if ( (n = read(child_parent_pipe[0], raw_response, MAXLINE)) < 0) {
            printf("Error reading from CGI script.\n");
            return build_response_500();
        }
        if (n == 0) {
            printf("Reading nothing from CGI script.\n");
            return build_response_500();
        }
        raw_response[n] = '\0';

        return build_raw_response(raw_response);
    }
    return build_response_500();
}