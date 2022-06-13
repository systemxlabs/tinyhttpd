//
// Created by Linwei Zhang on 2022/6/12.
//

#ifndef TINYHTTPD_FCGI_H
#define TINYHTTPD_FCGI_H

// FCGI_Header type
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11

struct fcgi_header_t {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
};

struct fcgi_record_t {
    struct fcgi_header_t header;
    char *content;
};

#endif //TINYHTTPD_FCGI_H
