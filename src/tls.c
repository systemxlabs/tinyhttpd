//
// Created by Linwei Zhang on 2022/6/15.
//

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "openssl/rand.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"
#include "tls.h"

/******************************************************************************
 * socket相关函数包装
 *****************************************************************************/
int tls_accept(int server_sockfd) {
    // 接收客户端连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);
    if (client_sockfd < 0) {
        perror("accept conn failed.\n");
        exit(1);
    }

    // 创建并初始化tls context
    tls_context_t *context = malloc(sizeof(tls_context_t));
    context->client_sockfd = client_sockfd;
    context->cert_filepath = "/Users/lewis/Github/tinyhttpd/cert/certificate.pem";
    context->key_filepath = "/Users/lewis/Github/tinyhttpd/cert/key.pem";

    // 执行 tls 握手
    if (tls_handshake(context)) {
        perror("tls handshake failed.\n");
        exit(1);
    }
    printf("Accept a new connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return client_sockfd;
}


/******************************************************************************
 * tls记录协议
 *****************************************************************************/
tls_record_t *tls_record_parse(uint8_t *record_data) {
    tls_record_t *record = (tls_record_t *)malloc(sizeof(tls_record_t));
    record->type = record_data[0];
    record->version = record_data[1] << 8 | record_data[2];
    record->length = record_data[3] << 8 | record_data[4];
    record->fragment = record_data + 5;
    printf("tls_record_parse type: %d, version: %d, length: %d\n", record->type, record->version, record->length);
    return record;
}

tls_record_t *tls_record_create(uint8_t type, uint16_t version, void *data) {
    tls_record_t *record = malloc(sizeof(tls_record_t));
    record->type = type;
    record->version = version;
    if (type == TLS_RECORD_CONTENT_TYPE_HANDSHAKE) {
        tls_handshake_t *handshake = (tls_handshake_t *)data;
        // 计算握手数据长度
        record->length = tls_handshake_length(handshake);
        // 拷贝握手数据
        record->fragment = malloc(record->length);
        record->fragment[0] = handshake->type;
        memcpy(record->fragment + 1, handshake->length, 3);
        uint32_t handshake_data_length = handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2];
        memcpy(record->fragment + 4, handshake->data, handshake_data_length);
        // print_bytes(record->data, record->length);
    }
    printf("tls_record_create type: %02x, version: %02x, length: %02x\n", record->type, record->version, record->length);
    return record;
}

uint16_t tls_record_length(tls_record_t *record) {
    uint16_t length = sizeof(record->type)
                    + sizeof(record->version)
                    + sizeof(record->length)
                    + record->length;
    return length;
}

int tls_record_send(tls_context_t *context, tls_record_t *record) {
    size_t raw_record_length = tls_record_length(record);
    uint8_t *raw_record = malloc(raw_record_length);
    raw_record[0] = record->type;
    raw_record[1] = record->version >> 8;
    raw_record[2] = record->version & 0xff;
    raw_record[3] = record->length >> 8;
    raw_record[4] = record->length & 0xff;
    memcpy(raw_record + 5, record->fragment, record->length);
    // print_bytes(raw_record, raw_record_length);
    if (send(context->client_sockfd, raw_record, raw_record_length, 0) < 0) {
        perror("send record failed.\n");
        exit(1);
    }
}


/******************************************************************************
 * tls握手协议
 *****************************************************************************/
int tls_handshake(tls_context_t *context) {
    uint8_t raw_record[1024];
    if (recv(context->client_sockfd, raw_record, sizeof(raw_record), 0) < 0) {
        perror("recv raw_record failed.\n");
        exit(1);
    }
    print_bytes(raw_record, sizeof(raw_record));
    // 解析记录协议
    tls_record_t *record = tls_record_parse(raw_record);
    if (record->type == TLS_RECORD_CONTENT_TYPE_HANDSHAKE) {
        // 解析握手协议
        tls_handshake_t *handshake = tls_handshake_parse(record->fragment);
        if (handshake->type == TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
            // 解析握手协议ClientHello
            tls_client_hello_t *client_hello = tls_client_hello_parse(handshake->data);

            // 创建并发送ServerHello
            tls_server_hello_t *server_hello = tls_server_hello_create(client_hello);
            tls_server_hello_send(context, server_hello);

            // 更新context
            context->server_random = server_hello->random;
            context->session_id = server_hello->session_id;
            context->cipher_suite = server_hello->cipher_suite;
            context->compression_method = server_hello->compression_method;

            // 创建并发送ServerCertificate
            tls_server_certificate_t *server_certificate = tls_server_certificate_create(context);
            tls_server_certificate_send(context, server_certificate);
        }
    }
    return 0;
}

tls_handshake_t *tls_handshake_parse(uint8_t *handshake_data) {
    tls_handshake_t *handshake = (tls_handshake_t *)malloc(sizeof(tls_handshake_t));
    handshake->type = handshake_data[0];
    handshake->length[0] = handshake_data[1];
    handshake->length[1] = handshake_data[2];
    handshake->length[2] = handshake_data[3];
    handshake->data = handshake_data + 4;
    printf("tls_handshake_parse type: %d, length: %d\n", handshake->type,
           handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);
    return handshake;
}

tls_handshake_t *tls_handshake_create(uint8_t type, void *data) {
    tls_handshake_t *handshake = malloc(sizeof(tls_handshake_t));
    handshake->type = type;

    if (type == TLS_HANDSHAKE_TYPE_SERVER_HELLO) {
        tls_server_hello_t *server_hello = (tls_server_hello_t *)data;

        // 计算server hello数据长度
        uint32_t handshake_data_length = tls_server_hello_length(server_hello);
        uint8_t *length_ptr = (uint8_t *)&handshake_data_length;
        handshake->length[0] = length_ptr[2];
        handshake->length[1] = length_ptr[1];
        handshake->length[2] = length_ptr[0];

        // 拷贝server hello数据
        handshake->data = malloc(handshake_data_length);
        uint8_t *data_ptr = handshake->data;
        data_ptr[0] = server_hello->version >> 8;
        data_ptr[1] = server_hello->version & 0xff;
        data_ptr += 2;
        data_ptr[0] = server_hello->random.gmt_unix_time >> 24;
        data_ptr[1] = server_hello->random.gmt_unix_time >> 16;
        data_ptr[2] = server_hello->random.gmt_unix_time >> 8;
        data_ptr[3] = server_hello->random.gmt_unix_time & 0xff;
        data_ptr += 4;
        memcpy(data_ptr, server_hello->random.random_bytes, TLS_RANDOM_BYTES_LEN);
        data_ptr += TLS_RANDOM_BYTES_LEN;
        data_ptr[0] = server_hello->session_id.session_id_length;
        data_ptr += 1;
        if (server_hello->session_id.session_id_length > 0) {
            memcpy(data_ptr, server_hello->session_id.session_id, server_hello->session_id.session_id_length);
            data_ptr += server_hello->session_id.session_id_length;
        }
        data_ptr[0] = server_hello->cipher_suite >> 8;
        data_ptr[1] = server_hello->cipher_suite & 0xff;
        data_ptr += 2;
        data_ptr[0] = server_hello->compression_method;
        data_ptr += 1;
        data_ptr[0] = server_hello->extensions.extensions_length >> 8;
        data_ptr[1] = server_hello->extensions.extensions_length & 0xff;
        data_ptr += 2;
        if (server_hello->extensions.extensions_length > 0) {
            memcpy(data_ptr, server_hello->extensions.extensions, server_hello->extensions.extensions_length);
            data_ptr += server_hello->extensions.extensions_length;
        }
        // print_bytes(handshake->data, handshake_data_length);
    }
    if (type == TLS_HANDSHAKE_TYPE_CERTIFICATE) {
        tls_server_certificate_t *server_certificate = (tls_server_certificate_t *)data;

        // 计算ServerCertificate数据长度
        uint32_t handshake_data_length = tls_server_certificate_length(server_certificate);
        uint8_t *length_ptr = (uint8_t *)&handshake_data_length;
        handshake->length[0] = length_ptr[2];
        handshake->length[1] = length_ptr[1];
        handshake->length[2] = length_ptr[0];
        printf("server_certificate length: %d\n", handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);

        // 拷贝ServerCertificate数据
        handshake->data = malloc(handshake_data_length);
        uint8_t *data_ptr = handshake->data;
        data_ptr[0] = server_certificate->certificates_length[0];
        data_ptr[1] = server_certificate->certificates_length[1];
        data_ptr[2] = server_certificate->certificates_length[2];
        data_ptr += 3;

        // 依次读取每个证书
        uint32_t certificates_length = server_certificate->certificates_length[0] << 16 |
                                        server_certificate->certificates_length[1] << 8 |
                                        server_certificate->certificates_length[2];
        printf("所有证书总长度（包含长度本身）: %d\n", certificates_length);
        uint8_t *certificate_data_ptr = (uint8_t *) server_certificate->data;
        uint32_t read_length = 0;
        while (read_length < certificates_length) {
            // 读取一个证书
            memcpy(data_ptr, certificate_data_ptr, 3);  // 读取一个证书长度
            uint32_t certificate_length = certificate_data_ptr[0] << 16 | certificate_data_ptr[1] << 8 | certificate_data_ptr[2];
            data_ptr += 3;
            read_length += 3;
            certificate_data_ptr += 3;
            memcpy(data_ptr, certificate_data_ptr, certificate_length);  // 读取一个证书内容
            data_ptr += certificate_length;
            read_length += certificate_length;
            certificate_data_ptr += certificate_length;
        }
        print_bytes(handshake->data, handshake_data_length);
    }
    printf("tls_handshake_create type: %02x, length: %02x, %02x, %02x\n", handshake->type, handshake->length[0], handshake->length[1], handshake->length[2]);
    return handshake;
}

uint16_t tls_handshake_length(tls_handshake_t *handshake) {
    uint16_t length = sizeof(handshake->type)
                    + sizeof(handshake->length)
                    + (handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);
    return length;
}


/******************************************************************************
 * ClientHello
 *****************************************************************************/
tls_client_hello_t *tls_client_hello_parse(uint8_t *client_hello_data) {
    tls_client_hello_t *client_hello = malloc(sizeof(tls_client_hello_t));

    uint8_t pos = 0;
    client_hello->version = client_hello_data[pos] << 8 | client_hello_data[pos + 1];
    pos += 2;

    // 解析随机数
    client_hello->random.gmt_unix_time = client_hello_data[pos] << 24 | client_hello_data[pos + 1] << 16 | client_hello_data[pos + 2] << 8 | client_hello_data[pos + 3];
    pos += 4;
    memcpy(client_hello->random.random_bytes, client_hello_data + pos, TLS_RANDOM_BYTES_LEN);
    pos += TLS_RANDOM_BYTES_LEN;

    // 解析session id
    client_hello->session_id.session_id_length = client_hello_data[pos];
    pos += 1;
    if (client_hello->session_id.session_id_length > 0) {
        memcpy(client_hello->session_id.session_id, client_hello_data + pos, client_hello->session_id.session_id_length);
        pos += client_hello->session_id.session_id_length;
    }

    // 解析CipherSuites
    client_hello->cipher_suites.cipher_suites_length = client_hello_data[pos] << 8 | client_hello_data[pos + 1];
    pos += 2;
    client_hello->cipher_suites.cipher_suites = (uint16_t *)malloc(client_hello->cipher_suites.cipher_suites_length);
    memcpy(client_hello->cipher_suites.cipher_suites, client_hello_data + pos, client_hello->cipher_suites.cipher_suites_length);
    pos += client_hello->cipher_suites.cipher_suites_length;

    // 解析CompressionMethods
    client_hello->compression_methods.compression_methods_length = client_hello_data[pos];
    pos += 1;
    client_hello->compression_methods.compression_methods = (uint8_t *)malloc(client_hello->compression_methods.compression_methods_length);
    memcpy(client_hello->compression_methods.compression_methods, client_hello_data + pos, client_hello->compression_methods.compression_methods_length);
    pos += client_hello->compression_methods.compression_methods_length;

    // 解析Extensions TODO 测试是否有问题
    client_hello->extensions.extensions_length = client_hello_data[pos] << 8 | client_hello_data[pos + 1];
    pos += 2;
    client_hello->extensions.extensions = (tls_extension_t *)malloc(client_hello->extensions.extensions_length);
    memcpy(client_hello->extensions.extensions, client_hello_data + pos, client_hello->extensions.extensions_length);
    pos += client_hello->extensions.extensions_length;

    printf("tls_client_hello_parse version: %02x, random.gmt_unix_time: %02x, session_id_length: %02x, cipher_suites_length: %02x, compression_methods_length: %02x, extensions_length: %02x\n",
           client_hello->version, client_hello->random.gmt_unix_time, client_hello->session_id.session_id_length,client_hello->cipher_suites.cipher_suites_length, client_hello->compression_methods.compression_methods_length, client_hello->extensions.extensions_length);
    return client_hello;
}


/******************************************************************************
 * ServerHello
 *****************************************************************************/
tls_server_hello_t *tls_server_hello_create(tls_client_hello_t *client_hello) {
    tls_server_hello_t *server_hello = malloc(sizeof(tls_server_hello_t));
    server_hello->version = TLS_PROTOCOL_VERSION_12;

    // openssl生成随机数
    server_hello->random.gmt_unix_time = time(NULL);
    RAND_bytes(server_hello->random.random_bytes, TLS_RANDOM_BYTES_LEN);

    // session id TODO 临时测试
    server_hello->session_id.session_id_length = 0;
//    server_hello->session_id.session_id = 0x01;

    // cipher suite TODO 临时测试
    server_hello->cipher_suite = TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_256_CBC_SHA256;

    // compression method TODO 临时测试
    server_hello->compression_method = TLS_COMPRESSION_METHOD_NULL;

    // extensions TODO 临时测试
    server_hello->extensions.extensions_length = 0;

    printf("tls_server_hello_create version: %02x, random.gmt_unix_time: %02x, session_id_length: %02x, cipher_suite: %02x, compression_method: %02x, extensions_length: %02x\n",
           server_hello->version, server_hello->random.gmt_unix_time, server_hello->session_id.session_id_length, server_hello->cipher_suite, server_hello->compression_method, server_hello->extensions.extensions_length);

    return server_hello;
}

uint32_t tls_server_hello_length(tls_server_hello_t *server_hello) {
    uint32_t length = sizeof(server_hello->version)
                    + sizeof(server_hello->random.gmt_unix_time) + sizeof(server_hello->random.random_bytes)
                    + sizeof(server_hello->session_id.session_id_length) + server_hello->session_id.session_id_length
                    + sizeof(server_hello->cipher_suite)
                    + sizeof(server_hello->compression_method)
                    + sizeof(server_hello->extensions.extensions_length) + server_hello->extensions.extensions_length;
    return length;
}

int tls_server_hello_send(tls_context_t *context, tls_server_hello_t *server_hello) {
    tls_handshake_t *handshake = tls_handshake_create(TLS_HANDSHAKE_TYPE_SERVER_HELLO, (uint8_t *)server_hello);
    tls_record_t *record = tls_record_create(TLS_RECORD_CONTENT_TYPE_HANDSHAKE, TLS_PROTOCOL_VERSION_12, handshake);
    return tls_record_send(context, record);
}


/******************************************************************************
 * ServerCertificate
 *****************************************************************************/
tls_server_certificate_t *tls_server_certificate_create(tls_context_t *context) {
    tls_server_certificate_t *server_certificate = malloc(sizeof(tls_server_certificate_t));

    tls_certificate_t *certificate = malloc(sizeof(tls_certificate_t));
    FILE *cert_fp = fopen(context->cert_filepath, "r");
    if (cert_fp == NULL) {
        printf("tls_server_certificate_create open cert file error\n");
        exit(1);
    }
    // 从pem格式文件读取证书
    X509 *cert = PEM_read_X509(cert_fp, NULL, NULL, NULL);
    context->cert = cert;
    // 将x509证书转换为der格式
    int cert_length = i2d_X509(cert, &certificate->certificate);
    uint8_t *cert_length_ptr = (uint8_t *)&cert_length;
    certificate->certificate_length[0] = cert_length_ptr[2];
    certificate->certificate_length[1] = cert_length_ptr[1];
    certificate->certificate_length[2] = cert_length_ptr[0];
    printf("单个证书长度：%d\n", cert_length);
    //print_bytes(certificate->certificate, certificate->certificate_length[0] << 16 | certificate->certificate_length[1] << 8 | certificate->certificate_length[2]);

    // 计算所有证书长度和
    uint32_t certificates_length = sizeof(certificate->certificate_length)
                                   + (certificate->certificate_length[0] << 16 | certificate->certificate_length[1] << 8 | certificate->certificate_length[2]);
    uint8_t *certificates_length_ptr = (uint8_t *)&certificates_length;
    server_certificate->certificates_length[0] = certificates_length_ptr[2];
    server_certificate->certificates_length[1] = certificates_length_ptr[1];
    server_certificate->certificates_length[2] = certificates_length_ptr[0];


    // 将证书复制到server_certificate
    server_certificate->data = (uint8_t *)malloc(certificates_length);
    uint8_t *data_ptr = server_certificate->data;
    memcpy(data_ptr, certificate->certificate_length, sizeof(certificate->certificate_length));
    data_ptr += sizeof(certificate->certificate_length);
    memcpy(data_ptr, certificate->certificate, cert_length);

    return server_certificate;
}

uint32_t tls_server_certificate_length(tls_server_certificate_t *server_certificate) {
    uint32_t length = sizeof(server_certificate->certificates_length)
            + (server_certificate->certificates_length[0] << 16 | server_certificate->certificates_length[1] << 8 | server_certificate->certificates_length[2]);
    return length;
}

int tls_server_certificate_send(tls_context_t *context, tls_server_certificate_t *server_certificate) {
    tls_handshake_t *handshake = tls_handshake_create(TLS_HANDSHAKE_TYPE_CERTIFICATE, (uint8_t *)server_certificate);
    tls_record_t *record = tls_record_create(TLS_RECORD_CONTENT_TYPE_HANDSHAKE, TLS_PROTOCOL_VERSION_12, handshake);
    return tls_record_send(context, record);
}


/******************************************************************************
 * 其他
 *****************************************************************************/
void print_bytes(uint8_t *data, size_t len) {
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
