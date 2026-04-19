#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static inline int greenc_net_init() {
    WSADATA w; return WSAStartup(MAKEWORD(2,2), &w);
}
typedef SOCKET greenc_sock;

#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int greenc_sock;
static inline int greenc_net_init() { return 0; }
#endif

static inline greenc_sock greenc_tcp_connect(const char* host, int port) {
    greenc_sock s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host); // keep simple (IP only)

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
    return s;
}

static inline int greenc_tcp_send(greenc_sock s, const char* buf, int len) {
#ifdef _WIN32
    return send(s, buf, len, 0);
#else
    return write(s, buf, len);
#endif
}

static inline int greenc_tcp_recv(greenc_sock s, char* buf, int len) {
#ifdef _WIN32
    return recv(s, buf, len, 0);
#else
    return read(s, buf, len);
#endif
}
