#ifndef PARSER_H
#define PARSER_H

#define MAX_HEADERS 32
#define MAX_PATH_LEN 1024
#define MAX_METHOD_LEN 8
#define MAX_HEADER_KEY 256
#define MAX_HEADER_VAL 512

#include <stddef.h>

typedef struct {
    char key[MAX_HEADER_KEY];
    char value[MAX_HEADER_VAL];
} HttpHeader;

typedef struct {
    char method[MAX_METHOD_LEN];      // "GET", "POST", "PUT", "DELETE"
    char path[MAX_PATH_LEN];          // "/users/42"
    char version[16];                 // "HTTP/1.1"
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char *body;                       // points into the original buffer
    size_t body_len;
} HttpRequest;

typedef enum {
    PARSE_OK,
    PARSE_ERROR_METHOD,
    PARSE_ERROR_PATH,
    PARSE_ERROR_VERSION,
    PARSE_ERROR_HEADER,
    PARSE_ERROR_INCOMPLETE
} ParseResult;



ParseResult parse_request(const char *buf, size_t len, HttpRequest *req);



const char *request_header(const HttpRequest *req, const char *key);

#endif