//
// Created by Linwei Zhang on 2022/6/13.
//

#ifndef TINYHTTPD_TLS_H
#define TINYHTTPD_TLS_H

int tls_start(int port);
void tls_stop(int sockfd);
int tls_accept_conn(int server_sockfd);
int tls_handle_conn(void *client_sockfd_ptr);
int tls_parse_client_hello(char *client_hello, int client_hello_len, char **server_hello, int *server_hello_len);

#endif //TINYHTTPD_TLS_H
