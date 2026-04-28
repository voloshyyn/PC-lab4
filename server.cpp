#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <winsock2.h>
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void recv_safe(SOCKET s, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int bytes = recv(s, buf + total, len - total, 0);
        if (bytes == SOCKET_ERROR || bytes == 0)
        {
            throw runtime_error("Втрачено з'єднання з клієнтом.");
        }
        total += bytes;
    }
}

void send_safe(SOCKET s, const char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int bytes = send(s, buf + total, len - total, 0);
        if (bytes == SOCKET_ERROR)
        {
            throw runtime_error("Помилка відправки даних.");
        }
        total += bytes;
    }
}
