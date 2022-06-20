//
// Created by Linwei Zhang on 2022/4/14.
//

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/hmac.h>
#include <sys/errno.h>
#include "openssl/rand.h"

void send_request_return_200(int sockfd) {
}
void send_request_return_404(int sockfd) {
}
void send_request_return_501(int sockfd) {
}


static int compute_prf(const EVP_MD *evp_md, uint8_t **pout, unsigned int size, char *key, char *seed)
{
    unsigned int ksize, count, ssize, buff_size;
    unsigned int hsize = EVP_MD_size(evp_md);
    uint8_t *buff, *prf;

    if (!evp_md || !pout || !size || !key || !seed)
        return -EINVAL;

    ssize = strnlen(seed, ~0U);
    buff = malloc(hsize + ssize);
    if (!buff)
        goto free_seed;

    *pout = prf = malloc(size + hsize);
    if (!prf)
        goto free_buff;

    ksize = strnlen(key, ~0U);
    memcpy(buff, seed, ssize);
    buff_size = ssize;

    for (count = 0; count < ROUND_UP(size, hsize); ++count) {
        HMAC(evp_md, key, ksize, buff, buff_size, buff, &buff_size);
        memcpy(buff + hsize, seed, ssize);

        HMAC(evp_md, key, ksize, buff, hsize + ssize, prf, NULL);
        prf += hsize;
    }

    free(buff);
    free(seed);
    return 0;

    free_buff:
    free(buff);
    free_seed:
    free(seed);

    return -ENOMEM;
}

int main() {
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket create failed");
        return 1;
    }

    // 2. 连接服务器
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_port = htons(8888); // 服务器端口
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 服务器的ip地址
    int ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret < 0) {
        perror("connect failed");
        return 1;
    }

    // 3. 发送数据
//    char raw_request[1024] = "GET /index.html HTTP/1.1\r\n"
//                        "Host: localhost:8080\r\n"
//                        "Connection: keep-alive\r\n"
//                        "Upgrade-Insecure-Requests: 1\r\n"
//                        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
//                        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3) AppleWebKit/604.5.6 (KHTML, like Gecko) Version/11.0.3 Safari/604.5.6\r\n"
//                        "Accept-Language: en-us\r\n"
//                        "DNT: 1\r\n"
//                        "Accept-Encoding: gzip, deflate\r\n"
//                        "\r\n"
//                        "Usually GET requests don\'t have a body\r\n"
//                        "But I don\'t care in this case :)";
    char raw_request[1024] = "GET /student.py?name=Tom HTTP/1.1\r\n"
                             "Host: localhost:8080\r\n"
                             "Connection: keep-alive\r\n"
                             "Upgrade-Insecure-Requests: 1\r\n"
                             "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                             "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_3) AppleWebKit/604.5.6 (KHTML, like Gecko) Version/11.0.3 Safari/604.5.6\r\n"
                             "Accept-Language: en-us\r\n"
                             "DNT: 1\r\n"
                             "Accept-Encoding: gzip, deflate\r\n"
                             "\r\n"
                             "Usually GET requests don\'t have a body\r\n"
                             "But I don\'t care in this case :)";
    size_t n = send(sockfd, raw_request, strlen(raw_request), 0);
    if (n < 0) {
        perror("send failed");
        return 1;
    }
    printf("send data: %s\n", raw_request);

    // 4. 读取服务器响应
    char buf[1024] = {0};
    if (recv(sockfd, buf, sizeof(buf), 0) < 0) {
        perror("recv failed");
        return 1;
    }
    printf("Message from server: %s\n", buf);


    // 5. 关闭套接字
    close(sockfd);
    return 0;
}