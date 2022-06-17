//
// Created by Linwei Zhang on 2022/6/13.
//

#ifndef TINYHTTPD_TLS_H
#define TINYHTTPD_TLS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "openssl/rand.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"

// TLS协议版本
#define TLS_PROTOCOL_VERSION_10 0x0301
#define TLS_PROTOCOL_VERSION_11 0x0302
#define TLS_PROTOCOL_VERSION_12 0x0303

// TLS记录类型
#define TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC 0x14
#define TLS_RECORD_CONTENT_TYPE_ALERT 0x15
#define TLS_RECORD_CONTENT_TYPE_HANDSHAKE 0x16
#define TLS_RECORD_CONTENT_TYPE_APPLICATION_DATA 0x17

// TLS握手类型
#define TLS_HANDSHAKE_TYPE_HELLO_REQUEST 0x00
#define TLS_HANDSHAKE_TYPE_CLIENT_HELLO 0x01
#define TLS_HANDSHAKE_TYPE_SERVER_HELLO 0x02
#define TLS_HANDSHAKE_TYPE_CERTIFICATE 0x0b
#define TLS_HANDSHAKE_TYPE_SERVER_KEY_EXCHANGE 0x0c
#define TLS_HANDSHAKE_TYPE_CERTIFICATE_REQUEST 0x0d
#define TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE 0x0e
#define TLS_HANDSHAKE_TYPE_CERTIFICATE_VERIFY 0x0f
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

#define TLS_CIPHER_SUITE_TLS_RSA_WITH_NULL_MD5                 0x0001
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_NULL_SHA                 0x0002
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_NULL_SHA256              0x003B
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_RC4_128_MD5              0x0004
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_RC4_128_SHA              0x0005
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_3DES_EDE_CBC_SHA         0x000A
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_128_CBC_SHA          0x002F
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_256_CBC_SHA          0x0035
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_128_CBC_SHA256       0x003C
#define TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_256_CBC_SHA256       0x003D

#define TLS_CIPHER_SUITE_TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA      0x000D
#define TLS_CIPHER_SUITE_TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA      0x0010
#define TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA     0x0013
#define TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA     0x0016
#define TLS_CIPHER_SUITE_TLS_DH_DSS_WITH_AES_128_CBC_SHA       0x0030
#define TLS_CIPHER_SUITE_TLS_DH_RSA_WITH_AES_128_CBC_SHA       0x0031
#define TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_AES_128_CBC_SHA      0x0032
#define TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_AES_128_CBC_SHA      0x0033
#define TLS_CIPHER_SUITE_TLS_DH_DSS_WITH_AES_256_CBC_SHA       0x0036
#define TLS_CIPHER_SUITE_TLS_DH_RSA_WITH_AES_256_CBC_SHA       0x0037
#define TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_AES_256_CBC_SHA      0x0038
#define TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_AES_256_CBC_SHA      0x0039
#define TLS_CIPHER_SUITE_TLS_DH_DSS_WITH_AES_128_CBC_SHA256    0x003E
#define TLS_CIPHER_SUITE_TLS_DH_RSA_WITH_AES_128_CBC_SHA256    0x003F
#define TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_AES_128_CBC_SHA256   0x0040
#define TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256   0x0067
#define TLS_CIPHER_SUITE_TLS_DH_DSS_WITH_AES_256_CBC_SHA256    0x0068
#define TLS_CIPHER_SUITE_TLS_DH_RSA_WITH_AES_256_CBC_SHA256    0x0069
#define TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_AES_256_CBC_SHA256   0x006A
#define TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256   0x006B

// TLS压缩算法
#define TLS_COMPRESSION_METHOD_NULL 0x00

typedef struct {
    uint8_t type;  // 数据类型, Application Data = 0x17
    uint16_t version;  // TLS版本, TLS 1.2 = 0x0303
    uint16_t length;  // 数据长度 2个字节
    uint8_t *fragment;  // 数据 fragment[length]
} tls_record_t;

typedef struct {
    uint8_t msg_type;  // 握手类型, Client Hello = 0x01
    uint8_t length[3];  // 握手长度 3个字节
    uint8_t *body;  // 握手 data[length]
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

typedef struct {
    uint8_t certificate_length[3];  // 证书长度 3个字节
    uint8_t *certificate;  // 证书 certificate[certificate_length] X.509v3证书
} tls_certificate_t;

typedef struct {
    uint8_t certificates_length[3];  // 证书总长度
    uint8_t *data;  // 所有证书内容 data[certificates_length]
} tls_server_certificate_t;

typedef struct {
} tls_server_hello_done_t;

typedef struct {
    uint16_t version;  // TLS版本
    uint8_t random[2 * TLS_RANDOM_BYTES_LEN];  // 随机数
} tls_pre_master_secret_t;

typedef struct {
    // TODO 还可能为ClientDiffieHellmanPublic
    uint16_t encrypted_pre_master_secret_length;  // pre_master_secret加密后的长度
    uint8_t *encrypted_pre_master_secret;  // pre_master_secret加密后的内容
} tls_client_key_exchange_t;

typedef struct {
    int client_sockfd; // 客户端套接字
    tls_random_t client_random;  // 客户端随机数
    tls_random_t server_random;  // 服务器随机数
    tls_session_id_t session_id;  // 会话ID
    uint16_t cipher_suite;  // 密钥套件
    uint8_t compression_method;  // 压缩算法
    //tls_extensions_t extensions;  // 扩展
    uint8_t enc_key_length;  // 加密密钥长度
    uint8_t mac_key_length;  // MAC密钥长度
    uint8_t mac_length;  // MAC长度
    char *pem_cert_filepath;  // 证书文件路径
    char *pem_key_filepath;  // 私钥文件路径
    X509 *cert;  // X.509v3证书
    tls_pre_master_secret_t pre_master_secret;  // pre_master_secret
} tls_context_t;

// socket相关包装函数
int tls_accept(int server_sockfd);
char *tls_recv(int client_sockfd);
int tls_send(int client_sockfd, char *msg);

// 记录协议
tls_record_t *tls_record_parse(uint8_t *record_data);
tls_record_t *tls_record_create(uint8_t type, uint16_t version, void *data);
uint16_t tls_record_length(tls_record_t *record);
int tls_record_send(tls_context_t *context, tls_record_t *record);

// 握手协议
int tls_handshake(tls_context_t *context);
tls_handshake_t *tls_handshake_parse(uint8_t *handshake_data);
tls_handshake_t *tls_handshake_create(uint8_t type, void *data);
uint16_t tls_handshake_length(tls_handshake_t *handshake);

// ClientHello
tls_client_hello_t *tls_client_hello_parse(uint8_t *client_hello_data);
uint32_t tls_client_hello_length(tls_client_hello_t *client_hello);
void tls_client_hello_free(tls_client_hello_t *client_hello);

// ServerHello
tls_server_hello_t *tls_server_hello_create(tls_client_hello_t *client_hello);
uint32_t tls_server_hello_length(tls_server_hello_t *server_hello);
void tls_server_hello_free(tls_server_hello_t *server_hello);
int tls_server_hello_send(tls_context_t *context, tls_server_hello_t *server_hello);

// ServerCertificate
tls_server_certificate_t *tls_server_certificate_create(tls_context_t *context);
uint32_t tls_server_certificate_length(tls_server_certificate_t *server_certificate);
void tls_server_certificate_free(tls_server_certificate_t *server_certificate);
int tls_server_certificate_send(tls_context_t *context, tls_server_certificate_t *server_certificate);
uint8_t *tls_server_certificate_files(tls_context_t *context, uint32_t *length);

// ServerKeyExchange
bool tls_server_key_exchange_needed(tls_context_t *context);

// CertificateRequest
bool tls_certificate_request_needed(tls_context_t *context);

// ServerHelloDone
tls_server_hello_done_t *tls_server_hello_done_create(tls_context_t *context);
int tls_server_hello_done_send(tls_context_t *context, tls_server_hello_done_t *server_hello_done);

// ClientKeyExchange
tls_client_key_exchange_t *tls_client_key_exchange_parse(uint8_t *client_key_exchange_data);

// 其他
void print_bytes(uint8_t *data, size_t len);

#endif //TINYHTTPD_TLS_H
