#include "quickjs-debugger.h"
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <windows.h>
#define	poll	WSAPoll
typedef int socklen_t;
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "jemalloc/jemalloc.h"

struct js_transport_data {
    int handle;
} js_transport_data;

static size_t js_transport_read(void *udata, char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL)
        return -3;

    int ret = recv(data->handle, (void *)buffer, length, 0);
    if (ret < 0)
        return -4;

    if (ret == 0)
        return -5;

    if (ret > length)
        return -6;

    return ret;
}

static size_t js_transport_write(void *udata, const char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL)
        return -3;

    size_t ret = send(data->handle, (const void *) buffer, length, 0);
    if (ret <= 0 || ret > (int) length)
        return -4;

    return ret;
}

static size_t js_transport_peek(void *udata) {
    struct pollfd fds[1];
    int poll_rc;

    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;

    fds[0].fd = data->handle;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    poll_rc = poll(fds, 1, 0);
    if (poll_rc < 0)
        return -2;
    if (poll_rc > 1)
        return -3;
    // no data
    if (poll_rc == 0)
        return 0;
    // has data
    return 1;
}

static void js_transport_close(JSContext* ctx, void *udata) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return;
    closesocket(data->handle);
    data->handle = 0;
    je_free(udata);
}

// todo: fixup asserts to return errors.
static struct sockaddr_in js_debugger_parse_sockaddr(const char* address) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    char* port_string = strstr(address, ":");
    assert(port_string);

    int port = atoi(port_string + 1);
    assert(port);

    char host_string[256];
    strcpy(host_string, address);
    host_string[port_string - address] = 0;
    if (host_string[0] == 'l')
    {
        memset(host_string, 0, sizeof(host_string));
        strcpy(host_string, "127.0.0.1");
    }

    struct hostent *host = gethostbyname(host_string);
    assert(host);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char *)&addr.sin_addr.s_addr, (char *)host->h_addr, host->h_length);
    addr.sin_port = htons(port);
    return addr;
}
#ifndef DISQJSD
void js_debugger_connect(JSContext *ctx, const char *address) {
    struct sockaddr_in addr = js_debugger_parse_sockaddr(address);
    int client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client > 0);

    connect(client, (const struct sockaddr *)&addr, sizeof(addr));

    struct js_transport_data *data = (struct js_transport_data *)je_malloc(sizeof(struct js_transport_data));
    memset(data, 0, sizeof(js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}

void js_debugger_wait_connection(JSContext *ctx, const char* address) {
    struct sockaddr_in addr = js_debugger_parse_sockaddr(address);
    int server = socket(AF_INET, SOCK_STREAM, 0);
    assert(server >= 0);

    int reuseAddress = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseAddress, sizeof(reuseAddress));

    bind(server, (struct sockaddr *) &addr, sizeof(addr));

    listen(server, 1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = (socklen_t) sizeof(addr);
    int client = accept(server, (struct sockaddr *) &client_addr, &client_addr_size);
    closesocket(server);
    assert(client >= 0);

    struct js_transport_data *data = (struct js_transport_data *)je_malloc(sizeof(struct js_transport_data));
    memset(data, 0, sizeof(js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}
#endif
#endif