#pragma once
#include "../../sys/net.h"
#include "../../sys/builtin/strings.h"
#include "../../sys/builtin/alloc.h"

static char* greenc_http_get(const char* host, int port, const char* path) {

    greenc_net_init();
    greenc_sock s = greenc_tcp_connect(host, port);
    if (s < 0) return 0;

    char* req = greenc_concat(
        "GET ", path, " HTTP/1.1\r\n",
        "Host: ", host, "\r\n",
        "Connection: close\r\n\r\n"
    );

    greenc_tcp_send(s, req, greenc_strlen(req));

    char* buf = (char*)greenc_malloc(8192);
    int n = greenc_tcp_recv(s, buf, 8191);
    if (n <= 0) return 0;

    buf[n] = 0;
    return buf;
}
