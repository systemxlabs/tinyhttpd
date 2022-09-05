//
// Created by Linwei Zhang on 2022/6/15.
//

#include <assert.h>
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
    tls_context_t *context = tls_context_init();
    context->client_sockfd = client_sockfd;

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
int tls_record_read(tls_context_t *context) {
    uint8_t raw_record[1024];
    int record_buf_len;
    if ((record_buf_len = recv(context->client_sockfd, raw_record, sizeof(raw_record), 0)) < 0) {
        perror("recv raw_record failed.\n");
        exit(1);
    }
    if (context->record_buf != NULL) {
        free(context->record_buf);
        context->record_buf = NULL;
        context->record_buf_len = 0;
    }
    context->record_buf = raw_record;
    context->record_buf_len = record_buf_len;
    print_bytes(context->record_buf, context->record_buf_len);
}

tls_record_t *tls_record_parse(tls_context_t *context) {
    print_bytes(context->record_buf, context->record_buf_len);
    tls_record_t *record = (tls_record_t *)malloc(sizeof(tls_record_t));
    uint8_t *buf = context->record_buf;
    record->type = buf[0];
    record->version = buf[1] << 8 | buf[2];
    record->length = buf[3] << 8 | buf[4];
    record->fragment = malloc(record->length);
    memcpy(record->fragment, buf + 5, record->length);

    uint16_t record_len = tls_record_length(record);
    context->record_buf_len -= record_len;
    if (context->record_buf_len > 0) {
        context->record_buf += record_len;
    } else {
        context->record_buf = NULL;
    }

    printf("tls_record_parse type: %02x, version: %02x, length: %02x\n", record->type, record->version, record->length);
    print_bytes(context->record_buf, context->record_buf_len);
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
        record->fragment[0] = handshake->msg_type;
        memcpy(record->fragment + 1, handshake->length, 3);
        uint32_t handshake_data_length = handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2];
        memcpy(record->fragment + 4, handshake->body, handshake_data_length);
        // print_bytes(record->data, record->length);
    }
    if (type == TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC) {
        tls_change_cipher_spec_t *change_cipher_spec = (tls_change_cipher_spec_t *)data;
        record->length = tls_change_cipher_spec_length(change_cipher_spec);
        record->fragment = malloc(record->length);
        record->fragment[0] = change_cipher_spec->type;
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
    tls_record_read(context);
    // 解析记录协议
    tls_record_t *record = tls_record_parse(context);
    if (record->type == TLS_RECORD_CONTENT_TYPE_HANDSHAKE) {
        // 解析握手协议
        tls_handshake_t *handshake = tls_handshake_parse(record->fragment);
        if (handshake->msg_type == TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
            // 解析握手协议ClientHello
            tls_client_hello_t *client_hello = tls_client_hello_parse(context, handshake->body);

            // 创建并发送ServerHello
            tls_server_hello_t *server_hello = tls_server_hello_create(context, client_hello);
            tls_server_hello_send(context, server_hello);

            // 更新context
            context->server_random = server_hello->random;
            context->session_id = server_hello->session_id;
            context->cipher_suite = server_hello->cipher_suite;
            context->compression_method = server_hello->compression_method;

            // 创建并发送ServerCertificate
            tls_server_certificate_t *server_certificate = tls_server_certificate_create(context);
            tls_server_certificate_send(context, server_certificate);

            // 创建并发送ServerKeyExchange
            if (tls_server_key_exchange_needed(context)) {
                // TODO
            }

            // 创建并发送CertificateRequest
            if (tls_certificate_request_needed(context)) {
                // TODO
            }

            // 创建并发送ServerHelloDone
            tls_server_hello_done_t *server_hello_done = tls_server_hello_done_create(context);
            tls_server_hello_done_send(context, server_hello_done);

            // 读取客户端数据
            if (context->record_buf_len <= 0) {
                tls_record_read(context);
            }

            // 解析记录协议
            tls_record_t *new_record = tls_record_parse(context);
            if (new_record->type == TLS_RECORD_CONTENT_TYPE_HANDSHAKE) {
                // 解析握手协议
                tls_handshake_t *new_handshake = tls_handshake_parse(new_record->fragment);

                if (new_handshake->msg_type == TLS_HANDSHAKE_TYPE_CLIENT_KEY_EXCHANGE) {
                    // 解析握手协议ClientKeyExchange
                    tls_client_key_exchange_t *client_key_exchange = tls_client_key_exchange_parse(new_handshake->body);

                    // print_bytes(client_key_exchange->encrypted_pre_master_secret, client_key_exchange->encrypted_pre_master_secret_length);
                    tls_client_key_exchange_decrypt(context, client_key_exchange);

                    // 计算master_secret
//                    tls_master_secret_compute(context);

                    // 计算key_block
//                    tls_key_block_compute(context);
                }
            }

            // 读取客户端数据
            if (context->record_buf_len <= 0) {
                tls_record_read(context);
            }
            tls_record_t *new_record2 = tls_record_parse(context);

            if (new_record2->type == TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC) {
                // 解析ChangeCipherSpec
                tls_change_cipher_spec_t *client_change_cipher_spec = tls_change_cipher_spec_parse(new_record2->fragment);
                printf("change_cipher_spec: %02x\n", client_change_cipher_spec->type);
                context->client_change_cipher_spec_type = client_change_cipher_spec->type;

                // 发送ChangeCipherSpec
                tls_change_cipher_spec_t *server_change_cipher_spec = tls_change_cipher_spec_create(TLS_CHANGE_CIPHER_SPEC_TYPE_1);
                tls_change_cipher_spec_send(context, server_change_cipher_spec);
            }

            // 读取客户端数据
            if (context->record_buf_len <= 0) {
                tls_record_read(context);
            }
            tls_record_t *new_record3 = tls_record_parse(context);

            if (new_record3->type == TLS_RECORD_CONTENT_TYPE_HANDSHAKE) {
                // 解析握手协议
                tls_handshake_t *new_handshake = tls_handshake_parse(new_record3->fragment);

//                if (new_handshake->msg_type == TLS_HANDSHAKE_TYPE_FINISHED) {
//                    // 解析握手协议Finished
//                    tls_finished_t *finished = tls_finished_parse(new_handshake->body);
//                    printf("finished: %02x\n", finished->verify_data_length);
//                    printf("finished: %02x\n", finished->verify_data[0]);
//                }
            }
        }
    }
    return 0;
}

tls_handshake_t *tls_handshake_parse(uint8_t *handshake_data) {
    tls_handshake_t *handshake = (tls_handshake_t *)malloc(sizeof(tls_handshake_t));
    handshake->msg_type = handshake_data[0];
    handshake->length[0] = handshake_data[1];
    handshake->length[1] = handshake_data[2];
    handshake->length[2] = handshake_data[3];
    handshake->body = handshake_data + 4;
    printf("tls_handshake_parse type: %d, length: %d\n", handshake->msg_type,
           handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);
    return handshake;
}

tls_handshake_t *tls_handshake_create(uint8_t type, void *data) {
    tls_handshake_t *handshake = malloc(sizeof(tls_handshake_t));
    handshake->msg_type = type;

    if (type == TLS_HANDSHAKE_TYPE_SERVER_HELLO) {
        tls_server_hello_t *server_hello = (tls_server_hello_t *)data;

        // 计算server hello数据长度
        uint32_t handshake_body_length = tls_server_hello_length(server_hello);
        uint8_t *length_ptr = (uint8_t *)&handshake_body_length;
        handshake->length[0] = length_ptr[2];
        handshake->length[1] = length_ptr[1];
        handshake->length[2] = length_ptr[0];

        // 拷贝server hello数据
        handshake->body = malloc(handshake_body_length);
        uint8_t *body_ptr = handshake->body;
        body_ptr[0] = server_hello->version >> 8;
        body_ptr[1] = server_hello->version & 0xff;
        body_ptr += 2;
        body_ptr[0] = server_hello->random.gmt_unix_time >> 24;
        body_ptr[1] = server_hello->random.gmt_unix_time >> 16;
        body_ptr[2] = server_hello->random.gmt_unix_time >> 8;
        body_ptr[3] = server_hello->random.gmt_unix_time & 0xff;
        body_ptr += 4;
        memcpy(body_ptr, server_hello->random.random_bytes, TLS_CLIENT_SERVER_RANDOM_BYTES_LEN);
        body_ptr += TLS_CLIENT_SERVER_RANDOM_BYTES_LEN;
        body_ptr[0] = server_hello->session_id.session_id_length;
        body_ptr += 1;
        if (server_hello->session_id.session_id_length > 0) {
            memcpy(body_ptr, server_hello->session_id.session_id, server_hello->session_id.session_id_length);
            body_ptr += server_hello->session_id.session_id_length;
        }
        body_ptr[0] = server_hello->cipher_suite >> 8;
        body_ptr[1] = server_hello->cipher_suite & 0xff;
        body_ptr += 2;
        body_ptr[0] = server_hello->compression_method;
        body_ptr += 1;
        body_ptr[0] = server_hello->extensions.extensions_length >> 8;
        body_ptr[1] = server_hello->extensions.extensions_length & 0xff;
        body_ptr += 2;
        if (server_hello->extensions.extensions_length > 0) {
            memcpy(body_ptr, server_hello->extensions.extensions, server_hello->extensions.extensions_length);
            body_ptr += server_hello->extensions.extensions_length;
        }
        // print_bytes(handshake->data, handshake_data_length);
    }
    if (type == TLS_HANDSHAKE_TYPE_CERTIFICATE) {
        tls_server_certificate_t *server_certificate = (tls_server_certificate_t *)data;

        // 计算ServerCertificate数据长度
        uint32_t handshake_body_length = tls_server_certificate_length(server_certificate);
        uint8_t *length_ptr = (uint8_t *)&handshake_body_length;
        handshake->length[0] = length_ptr[2];
        handshake->length[1] = length_ptr[1];
        handshake->length[2] = length_ptr[0];
        printf("server_certificate length: %d\n", handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);

        // 拷贝ServerCertificate数据
        handshake->body = malloc(handshake_body_length);
        uint8_t *body_ptr = handshake->body;
        body_ptr[0] = server_certificate->certificates_length[0];
        body_ptr[1] = server_certificate->certificates_length[1];
        body_ptr[2] = server_certificate->certificates_length[2];
        body_ptr += 3;

        // 依次读取每个证书
        uint32_t certificates_length = server_certificate->certificates_length[0] << 16 |
                                        server_certificate->certificates_length[1] << 8 |
                                        server_certificate->certificates_length[2];
        printf("所有证书总长度（包含长度本身）: %d\n", certificates_length);
        uint8_t *certificate_data_ptr = (uint8_t *) server_certificate->data;
        uint32_t read_length = 0;
        while (read_length < certificates_length) {
            // 读取一个证书
            memcpy(body_ptr, certificate_data_ptr, 3);  // 读取一个证书长度
            uint32_t certificate_length = certificate_data_ptr[0] << 16 | certificate_data_ptr[1] << 8 | certificate_data_ptr[2];
            body_ptr += 3;
            read_length += 3;
            certificate_data_ptr += 3;
            memcpy(body_ptr, certificate_data_ptr, certificate_length);  // 读取一个证书内容
            body_ptr += certificate_length;
            read_length += certificate_length;
            certificate_data_ptr += certificate_length;
        }
        // print_bytes(handshake->data, handshake_data_length);
    }
    if (type == TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE) {
        handshake->length[0] = 0;
        handshake->length[1] = 0;
        handshake->length[2] = 0;
    }
    printf("tls_handshake_create type: %02x, length: %02x, %02x, %02x\n", handshake->msg_type, handshake->length[0], handshake->length[1], handshake->length[2]);
    return handshake;
}

uint16_t tls_handshake_length(tls_handshake_t *handshake) {
    uint16_t length = sizeof(handshake->msg_type)
                    + sizeof(handshake->length)
                    + (handshake->length[0] << 16 | handshake->length[1] << 8 | handshake->length[2]);
    return length;
}


/******************************************************************************
 * ClientHello
 *****************************************************************************/
tls_client_hello_t *tls_client_hello_parse(tls_context_t *context, uint8_t *client_hello_data) {
    tls_client_hello_t *client_hello = malloc(sizeof(tls_client_hello_t));

    uint8_t pos = 0;
    client_hello->version = client_hello_data[pos] << 8 | client_hello_data[pos + 1];
    pos += 2;

    // 解析随机数
    client_hello->random.gmt_unix_time = client_hello_data[pos] << 24 | client_hello_data[pos + 1] << 16 | client_hello_data[pos + 2] << 8 | client_hello_data[pos + 3];
    pos += 4;
    memcpy(client_hello->random.random_bytes, client_hello_data + pos, TLS_CLIENT_SERVER_RANDOM_BYTES_LEN);
    pos += TLS_CLIENT_SERVER_RANDOM_BYTES_LEN;

    context->client_random = client_hello->random;

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
tls_server_hello_t *tls_server_hello_create(tls_context_t *context, tls_client_hello_t *client_hello) {
    tls_server_hello_t *server_hello = malloc(sizeof(tls_server_hello_t));
    server_hello->version = TLS_PROTOCOL_VERSION_12;

    // openssl生成随机数
    server_hello->random.gmt_unix_time = time(NULL);
    RAND_bytes(server_hello->random.random_bytes, TLS_CLIENT_SERVER_RANDOM_BYTES_LEN);

    context->server_random = server_hello->random;

    // session id TODO 临时测试
    server_hello->session_id.session_id_length = 0;
//    server_hello->session_id.session_id = 0x01;

    // cipher suite TODO 临时测试
    server_hello->cipher_suite = TLS_CIPHER_SUITE_TLS_RSA_WITH_AES_256_CBC_SHA256;

    // compression method
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
    FILE *cert_fp = fopen(context->pem_cert_filepath, "r");
    if (cert_fp == NULL) {
        printf("tls_server_certificate_create open cert file error\n");
        exit(1);
    }
    // 从pem格式文件读取证书
    X509 *cert = PEM_read_X509(cert_fp, NULL, NULL, NULL);
//    context->cert = cert;
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
 * ServerKeyExchange
 *****************************************************************************/
bool tls_server_key_exchange_needed(tls_context_t *context) {
    uint16_t cipher_suite = context->cipher_suite;
    if (cipher_suite == TLS_CIPHER_SUITE_TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA
        || cipher_suite == TLS_CIPHER_SUITE_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA
        || cipher_suite == TLS_CIPHER_SUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256) {
        // TODO DHE_DSS、DHE_RSA、ECDHE_RSA、ECDHE_ECDSA系列套件 需要发送ServerKeyExchange
        return true;
    }
    return false;
}



/******************************************************************************
 * CertificateRequest
 *****************************************************************************/
bool tls_certificate_request_needed(tls_context_t *context) {
    return false;
}


/******************************************************************************
 * ServerHelloDone
 *****************************************************************************/
tls_server_hello_done_t *tls_server_hello_done_create(tls_context_t *context) {
    // ServerHelloDone消息为空
    tls_server_hello_done_t *server_hello_done = NULL;
    return server_hello_done;
}

int tls_server_hello_done_send(tls_context_t *context, tls_server_hello_done_t *server_hello_done) {
    tls_handshake_t *handshake = tls_handshake_create(TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE, (uint8_t *)server_hello_done);
    tls_record_t *record = tls_record_create(TLS_RECORD_CONTENT_TYPE_HANDSHAKE, TLS_PROTOCOL_VERSION_12, handshake);
    return tls_record_send(context, record);
}


/******************************************************************************
 * ClientKeyExchange
 *****************************************************************************/
tls_client_key_exchange_t *tls_client_key_exchange_parse(uint8_t *client_key_exchange_data) {
    tls_client_key_exchange_t *client_key_exchange = (tls_client_key_exchange_t *)malloc(sizeof(tls_client_key_exchange_t));

    // 解析encrypted_pre_master_secret_length
    uint8_t pos = 0;
    client_key_exchange->encrypted_pre_master_secret_length = client_key_exchange_data[pos] << 8 | client_key_exchange_data[pos + 1];
    pos += 2;

    // 解析encrypted_pre_master_secret
    client_key_exchange->encrypted_pre_master_secret = (uint8_t *)malloc(client_key_exchange->encrypted_pre_master_secret_length);
    memcpy(client_key_exchange->encrypted_pre_master_secret, client_key_exchange_data + pos, client_key_exchange->encrypted_pre_master_secret_length);

    return client_key_exchange;
}

void tls_client_key_exchange_decrypt(tls_context_t *context, tls_client_key_exchange_t *client_key_exchange) {
    // 解密encrypted_pre_master_secret
    uint8_t *encrypted_pre_master_secret = client_key_exchange->encrypted_pre_master_secret;
    uint8_t *decrypted_pre_master_secret = (uint8_t *)malloc(TLS_PRE_MASTER_SECRET_LEN);

    // 读取私钥
    FILE *key_fp = fopen(context->pem_key_filepath, "r");
    if (key_fp == NULL) {
        printf("tls_client_key_exchange_decrypt open key file error\n");
        exit(1);
    }
    RSA *rsa_private_key = PEM_read_RSAPrivateKey(key_fp, NULL, NULL, NULL);
    check_errors();

    print_bytes(context->record_buf, context->record_buf_len);

    int len = RSA_private_decrypt(client_key_exchange->encrypted_pre_master_secret_length,
                        encrypted_pre_master_secret,
                        decrypted_pre_master_secret,
                        rsa_private_key,
                        RSA_NO_PADDING);
    RSA_free(rsa_private_key);
    OPENSSL_thread_stop();
    CRYPTO_cleanup_all_ex_data();

    print_bytes(context->record_buf, context->record_buf_len);

    check_errors();
    if (len < 0) {
        printf("tls_client_key_exchange_decrypt RSA_private_decrypt error, encrypted_pre_master_secret_length: %d\n",
               client_key_exchange->encrypted_pre_master_secret_length);
        printf(ERR_error_string(ERR_get_error(), NULL));
        exit(1);
    }
    printf("pre master len: %d\n", len);
    // print_bytes(client_key_exchange->encrypted_pre_master_secret, client_key_exchange->encrypted_pre_master_secret_length);
    // print_bytes(decrypted_pre_master_secret, length);

//    EVP_PKEY_CTX *ctx;
//    unsigned char *out, *in;
//    size_t outlen, inlen;
//    EVP_PKEY *key;
//    ctx = EVP_PKEY_CTX_new(key, NULL);
//
//    if (EVP_PKEY_decrypt(ctx, out, &outlen, in, inlen) <= 0) {
//        printf("EVP_PKEY_decrypt error\n");
//        exit(1);
//    }

    //print_bytes(context->record_buf, context->record_buf_len);

    // 解析解密后的pre_master_secret
    tls_pre_master_secret_t *pre_master_secret = malloc(sizeof(tls_pre_master_secret_t));
    uint8_t *decrypted_pre_master_secret_ptr = decrypted_pre_master_secret;
    pre_master_secret->version = decrypted_pre_master_secret_ptr[0] << 8 | decrypted_pre_master_secret_ptr[1];
    decrypted_pre_master_secret_ptr += 2;
    memcpy(pre_master_secret->random, decrypted_pre_master_secret_ptr, TLS_PRE_MASTER_RANDOM_LEN);

    context->pre_master_secret = pre_master_secret;
}


/******************************************************************************
 * TLS Change Cipher Spec协议
 *****************************************************************************/
tls_change_cipher_spec_t *tls_change_cipher_spec_parse(uint8_t *change_cipher_spec_data) {
    tls_change_cipher_spec_t *change_cipher_spec = (tls_change_cipher_spec_t *)malloc(sizeof(tls_change_cipher_spec_t));
    change_cipher_spec->type = change_cipher_spec_data[0];
    return change_cipher_spec;
}

tls_change_cipher_spec_t *tls_change_cipher_spec_create(uint8_t type) {
    tls_change_cipher_spec_t *change_cipher_spec = (tls_change_cipher_spec_t *)malloc(sizeof(tls_change_cipher_spec_t));
    change_cipher_spec->type = type;
    return change_cipher_spec;
}

uint32_t tls_change_cipher_spec_length(tls_change_cipher_spec_t *change_cipher_spec) {
    return sizeof(change_cipher_spec->type);
}

int tls_change_cipher_spec_send(tls_context_t *context, tls_change_cipher_spec_t *change_cipher_spec) {
    tls_record_t *record = tls_record_create(TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC, TLS_PROTOCOL_VERSION_12, change_cipher_spec);
    return tls_record_send(context, record);
}


/******************************************************************************
 * 其他
 *****************************************************************************/
tls_context_t *tls_context_init() {
    tls_context_t *context = malloc(sizeof(tls_context_t));
    // TODO 临时设定绝对路径
    context->pem_cert_filepath = "/Users/lewis/Github/tinyhttpd/cert/certificate.pem";
    context->pem_key_filepath = "/Users/lewis/Github/tinyhttpd/cert/key.pem";
    // 读取私钥
//    FILE *key_fp = fopen(context->pem_key_filepath, "r");
//    if (key_fp == NULL) {
//        printf("tls_context_init open key file error\n");
//        exit(1);
//    }
//    RSA *rsa_private_key =  PEM_read_RSAPrivateKey(key_fp, NULL, NULL, NULL);
//    context->rsa_private_key = rsa_private_key;
//    fclose(key_fp);
    return context;
}

void tls_master_secret_compute(tls_context_t *context) {
    // 计算master_secret
    uint8_t *pre_master_secret = (uint8_t *)context->pre_master_secret;
    uint8_t *client_random = (uint8_t *)&context->client_random;
    uint8_t *server_random = (uint8_t *)&context->server_random;

    const char* label = "master secret";
    int seed_len = strlen(label) + TLS_CLIENT_SERVER_RANDOM_LEN * 2;
    unsigned char seed[seed_len];
    memcpy(seed, label, strlen(label));
    memcpy(&seed[strlen(label)], client_random, TLS_CLIENT_SERVER_RANDOM_LEN);
    memcpy(&seed[strlen(label) + TLS_CLIENT_SERVER_RANDOM_LEN], server_random, TLS_CLIENT_SERVER_RANDOM_LEN);
    uint8_t *master_secret = malloc(TLS_MASTER_SECRET_LEN);
    tls_prf_sha256(pre_master_secret, TLS_PRE_MASTER_SECRET_LEN, seed, seed_len, master_secret, TLS_MASTER_SECRET_LEN);
    context->master_secret = master_secret;

    //print_bytes(context->master_secret, TLS_MASTER_SECRET_LEN);
}

void tls_key_block_compute(tls_context_t *context) {
    // 计算key_block
    uint8_t *master_secret = context->master_secret;
    uint8_t *client_random = (uint8_t *)&context->client_random;
    uint8_t *server_random = (uint8_t *)&context->server_random;

    const char* label = "key expansion";
    int seed_len = strlen(label) + TLS_CLIENT_SERVER_RANDOM_LEN * 2;
    unsigned char seed[seed_len];
    memcpy(seed, label, strlen(label));
    memcpy(&seed[strlen(label)], client_random, TLS_CLIENT_SERVER_RANDOM_LEN);
    memcpy(&seed[strlen(label) + TLS_CLIENT_SERVER_RANDOM_LEN], server_random, TLS_CLIENT_SERVER_RANDOM_LEN);

    uint8_t *key_block = malloc(TLS_KEY_BLOCK_LEN);
    tls_prf_sha256(master_secret, TLS_MASTER_SECRET_LEN, seed, seed_len, key_block, TLS_KEY_BLOCK_LEN);
    context->key_block = key_block;

    //print_bytes(context->key_block, TLS_KEY_BLOCK_LEN);
}

void print_bytes(uint8_t *data, size_t len) {
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

/**
 * 计算RPF：PRF(secret,label,seed) = P_sha256(secret,label + seed)
 * @param key [in]  密数secret，如pre_master_secret
 * @param keylen [in]  密数secret的字节数
 * @param seed [in] label+seed合并后的数据，如 "master secret" + client random + server random
 * @param seedlen [in] label+seed合并后的数据字节数
 * @param pout [out] 输出区
 * @param outlen [in] 输出字节数,即需要扩展到的字节数。
 *
 * P_hash(secret, seed) =
 * HMAC_hash(secret, A(1) + seed) +
 * HMAC_hash(secret, A(2) + seed) +
 * HMAC_hash(secret, A(3) + seed) + ...
 * 其中+表示连接：
 * A() is defined as:
 * A(0) = seed
 * A(i) = HMAC_hash(secret, A(i-1))
*/
bool tls_prf_sha256(const uint8_t *key, int keylen, const uint8_t *seed, int seedlen, uint8_t *pout, int outlen) {
    int nout = 0;  // 输出字节数
    int hash_size = EVP_MD_size(EVP_sha256());  // 32

    uint8_t An_1[hash_size];  // A(n-1) = HMAC_hash(secret, A(n-2))
    uint8_t An[hash_size];  // A(n) = HMAC_hash(secret, A(n-1))

    uint8_t Aout[hash_size];  // Aout = HMAC_hash(secret, A(i) + seed)
    unsigned int mdlen = 0;  // mdlen是HMAC_hash的输出长度

    // 计算A(1) = HMAC_hash(secret, A(0)), n = 2
    if (!HMAC(EVP_sha256(), key, (int)keylen, seed, seedlen, An_1, &mdlen)) {
        return false;
    }

    uint8_t *ps = malloc(seedlen + hash_size);

    while (nout < outlen)
    {
        memcpy(ps, An_1, hash_size);  // 将A(n-1)复制到ps
        memcpy(ps + hash_size, seed, seedlen);  // 将seed复制到ps

        // 计算Aout = HMAC_hash(secret, A(n-1) + seed)
        if (!HMAC(EVP_sha256(), key, (int)keylen, ps, hash_size + seedlen, Aout, &mdlen)) {
            return false;
        }

        // 复制Aout到pout
        if (nout + hash_size < outlen) {
            memcpy(pout + nout, Aout, hash_size);
            nout += 32;
        } else {
            memcpy(pout + nout, Aout, outlen - nout);
            nout = outlen;
            break;
        }

        // 计算A(n) = HMAC_hash(secret, A(n-1))
        if (!HMAC(EVP_sha256(), key, (int)keylen, An_1, hash_size, An, &mdlen)) {
            return false;
        }
        // 复制An到An_1
        memcpy(An_1, An, hash_size);
    }
    return true;
}

void check_errors() {
    const int error = ERR_get_error();
    if ( error != 0 ) {
        printf("check error failed, reason: %s\n", ERR_reason_error_string( error ));
        assert(0);
    }
}