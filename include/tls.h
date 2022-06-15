//
// Created by Linwei Zhang on 2022/6/13.
//

#ifndef TINYHTTPD_TLS_H
#define TINYHTTPD_TLS_H

#include <stdint.h>

// TLS协议版本
#define TLS_PROTOCOL_VERSION_10 0x0301
#define TLS_PROTOCOL_VERSION_11 0x0302
#define TLS_PROTOCOL_VERSION_12 0x0303

// TLS记录 类型
#define TLS_RECORD_CONTENT_TYPE_HANDSHAKE 0x16
#define TLS_RECORD_CONTENT_TYPE_APPLICATION_DATA 0x17
#define TLS_RECORD_CONTENT_TYPE_ALERT 0x15
#define TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC 0x14

// TLS握手 类型
#define TLS_HANDSHAKE_TYPE_CLIENT_HELLO 0x01
#define TLS_HANDSHAKE_TYPE_SERVER_HELLO 0x02
#define TLS_HANDSHAKE_TYPE_CERTIFICATE 0x0b
#define TLS_HANDSHAKE_TYPE_SERVER_KEY_EXCHANGE 0x0c
#define TLS_HANDSHAKE_TYPE_CERTIFICATE_REQUEST 0x0d
#define TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE 0x0e
#define TLS_HANDSHAKE_TYPE_CERTIFICATE_VERIFY 0x12
#define TLS_HANDSHAKE_TYPE_CLIENT_KEY_EXCHANGE 0x10
#define TLS_HANDSHAKE_TYPE_FINISHED 0x14

// TLS会话ID最大长度
#define TLS_SESSION_ID_MAX_LEN 16

// TLS随机数长度
#define TLS_RANDOM_BYTES_LEN 28

// TLS密钥套件类型
#define TLS_CIPHER_SUITE_TLS_ASE_128_GCM_SHA256 0x1301
#define TLS_CIPHER_SUITE_TLS_ASE_256_GCM_SHA384 0x1302
#define TLS_CIPHER_SUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 0xc02f
#define TLS_CIPHER_SUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 0xc030

// TLS压缩算法
#define TLS_COMPRESSION_METHOD_NULL 0x00

typedef struct {
    uint8_t type;  // 数据类型, Application Data = 0x17
    uint16_t version;  // TLS版本, TLS 1.2 = 0x0303
    uint16_t length;  // 数据长度 2个字节
    uint8_t *data;  // 数据 data[length]
} tls_record_t;

typedef struct {
    uint8_t type;  // 握手类型, Client Hello = 0x01
    uint8_t length[3];  // 握手长度 3个字节
    uint8_t *data;  // 握手 data[length]
} tls_handshake_t;

typedef struct {
    uint16_t type;  // 扩展类型
    uint16_t length;  // 扩展长度
    uint8_t *data;  // 扩展数据 data[length]
} tls_extension_t;

typedef struct {
    uint16_t extensions_length; // 扩展总长度
    tls_extension_t *extensions; // 扩展总数据
} tls_extensions_t;

typedef struct {
    uint8_t session_id_length;  // 会话ID长度
    uint8_t *session_id;  // 会话ID
} tls_session_id_t;

typedef struct {
    uint16_t cipher_suites_length;  // 密钥套件长度
    uint16_t *cipher_suites;  // 密钥套件
} tls_cipher_suites_t;

typedef struct {
    uint8_t compression_methods_length;  // 压缩算法长度
    uint8_t *compression_methods;  // 压缩算法
} tls_compression_methods_t;

typedef struct {
    uint32_t gmt_unix_time;  // 生成时间
    uint8_t random_bytes[TLS_RANDOM_BYTES_LEN];  // 随机数
} tls_random_t;

typedef struct {
    uint16_t version;  // TLS版本
    tls_random_t random;  // 随机数
    tls_session_id_t session_id;  // 会话ID
    tls_cipher_suites_t cipher_suites;  // 密钥套件
    tls_compression_methods_t compression_methods;  // 压缩算法
    tls_extensions_t extensions;  // 扩展
} tls_client_hello_t;

typedef struct {
    uint16_t version;  // TLS版本
    tls_random_t random;  // 随机数
    tls_session_id_t session_id;  // 会话ID
    uint16_t cipher_suite;  // 密钥套件
    uint8_t compression_method;  // 压缩算法
    tls_extensions_t extensions;  // 扩展
} tls_server_hello_t;


// tls包装后的函数
int tls_accept(int server_sockfd);
int tls_handshake(int client_sockfd);
char *tls_recv(int client_sockfd);
int tls_send(int client_sockfd, char *msg);

tls_record_t *tls_record_parse(uint8_t *record_data);
tls_handshake_t *tls_handshake_parse(uint8_t *handshake_data);
tls_client_hello_t *tls_client_hello_parse(uint8_t *client_hello_data);

tls_server_hello_t *tls_server_hello_create(tls_client_hello_t *client_hello);
tls_handshake_t *tls_handshake_create(uint8_t type, void *data);
tls_record_t *tls_record_create(uint8_t type, uint16_t version, void *data);

int tls_server_hello_send(int client_sockfd, tls_server_hello_t *server_hello);
int tls_record_send(int client_sockfd, tls_record_t *record);

int tls_session_id_len(tls_session_id_t *session_id);
int tls_cipher_suites_len(tls_cipher_suites_t *cipher_suites);
int tls_compression_methods_len(tls_compression_methods_t *compression_methods);
int tls_extensions_len(tls_extensions_t *extensions);

void print_bytes(uint8_t *data, size_t len);

#endif //TINYHTTPD_TLS_H
