#include <iostream>
#include <vector>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <ctime>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void send_safe(SOCKET s, const char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int bytes = send(s, buf + total, len - total, 0);
        if (bytes == SOCKET_ERROR)
            throw runtime_error("Помилка відправки на сервер.");
        total += bytes;
    }
}

void recv_safe(SOCKET s, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int bytes = recv(s, buf + total, len - total, 0);
        if (bytes == SOCKET_ERROR || bytes == 0)
            throw runtime_error("Сервер розірвав з'єднання.");
        total += bytes;
    }
}