#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// finds \r\n in buf starting from position pos
// returns pointer to the \r, or NULL if not found
static const char *find_crlf(const char *buf, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return buf + i;
        }
    }
    return NULL;
}

//save the request line (method, path, version) into req struct
static ParseResult parse_request_line(const char *buf, size_t len,
                                       HttpRequest *req,
                                       const char **rest) {
    const char *crlf = find_crlf(buf, len);
    if (!crlf) return PARSE_ERROR_INCOMPLETE;

    // find method (up to first space)
    const char *space1 = memchr(buf, ' ', crlf - buf);
    if (!space1) return PARSE_ERROR_METHOD;

    size_t method_len = space1 - buf;
    if (method_len == 0 || method_len >= MAX_METHOD_LEN)
        return PARSE_ERROR_METHOD;

    memcpy(req->method, buf, method_len);
    req->method[method_len] = '\0';

    // find path (between first and second space)
    const char *path_start = space1 + 1;
    const char *space2 = memchr(path_start, ' ', crlf - path_start);
    if (!space2) return PARSE_ERROR_PATH;

    size_t path_len = space2 - path_start;
    if (path_len == 0 || path_len >= MAX_PATH_LEN)
        return PARSE_ERROR_PATH;

    memcpy(req->path, path_start, path_len);
    req->path[path_len] = '\0';

    // version is between second space and \r\n
    const char *version_start = space2 + 1;
    size_t version_len = crlf - version_start;
    if (version_len == 0 || version_len >= 16)
        return PARSE_ERROR_VERSION;

    memcpy(req->version, version_start, version_len);
    req->version[version_len] = '\0';

    // rest of buffer starts after \r\n
    *rest = crlf + 2;
    return PARSE_OK;
}



//parse headers and save them into req struct
static ParseResult parse_headers(const char *buf, size_t len,
                                  HttpRequest *req,
                                  const char **rest) {
    const char *cur = buf;
    const char *end = buf + len;
    req->header_count = 0;

    while (cur < end) {
        // empty line means end of headers
        if (cur[0] == '\r' && cur[1] == '\n') {
            *rest = cur + 2;
            return PARSE_OK;
        }

        const char *crlf = find_crlf(cur, end - cur);
        if (!crlf) return PARSE_ERROR_INCOMPLETE;

        // find the colon separating key from value
        const char *colon = memchr(cur, ':', crlf - cur);
        if (!colon) return PARSE_ERROR_HEADER;

        if (req->header_count >= MAX_HEADERS)
            return PARSE_ERROR_HEADER;

        HttpHeader *h = &req->headers[req->header_count++];

        // copy key
        size_t key_len = colon - cur;
        if (key_len == 0 || key_len >= MAX_HEADER_KEY)
            return PARSE_ERROR_HEADER;
        memcpy(h->key, cur, key_len);
        h->key[key_len] = '\0';

        // copy value (skip the space after the colon)
        const char *val_start = colon + 1;
        while (val_start < crlf && *val_start == ' ') val_start++;
        size_t val_len = crlf - val_start;
        if (val_len >= MAX_HEADER_VAL)
            return PARSE_ERROR_HEADER;
        memcpy(h->value, val_start, val_len);
        h->value[val_len] = '\0';

        cur = crlf + 2;
    }

    return PARSE_ERROR_INCOMPLETE;
}



static ParseResult parse_body(const char *buf, size_t len,
                               HttpRequest *req) {
    const char *content_length = request_header(req, "Content-Length");

    if (!content_length) {
        req->body = NULL;
        req->body_len = 0;
        return PARSE_OK;
    }

    size_t expected = (size_t)atol(content_length);

    if (len < expected)
        return PARSE_ERROR_INCOMPLETE;

    req->body = (char*)buf;
    req->body_len = expected;
    return PARSE_OK;
}

ParseResult parse_request(const char *buf, size_t len, HttpRequest *req) {
    memset(req, 0, sizeof(HttpRequest));

    const char *after_request_line;
    ParseResult result = parse_request_line(buf, len, req, &after_request_line);
    if (result != PARSE_OK) return result;

    const char *after_headers;
    size_t remaining = len - (after_request_line - buf);
    result = parse_headers(after_request_line, remaining, req, &after_headers);
    if (result != PARSE_OK) return result;

    remaining = len - (after_headers - buf);
    return parse_body(after_headers, remaining, req);
}

const char *request_header(const HttpRequest *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}