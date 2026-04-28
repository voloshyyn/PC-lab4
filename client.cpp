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

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    srand(time(0));
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    try
    {
        if (connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            throw runtime_error("Не вдалося підключитися до сервера.");
        }
        cout << "Підключено до сервера!" << endl;

        int n = 100;
        int num_threads = 4;
        Config cfg = {n, num_threads};

        vector<int> matrix(n * n);
        vector<int> vec(n);
        for (int i = 0; i < n * n; ++i)
            matrix[i] = rand() % 9 + 1;
        for (int i = 0; i < n; ++i)
            vec[i] = rand() % 9 + 1;

        Header req, res;

        cout << "Відправка матриці (" << n << "x" << n << ") та вектора..." << endl;
        int payload_size = sizeof(Config) + (n * n * sizeof(int)) + (n * sizeof(int));
        req = {CMD_SEND_DATA, payload_size};

        header_to_network(req);
        config_to_network(cfg);
        array_to_network(matrix);
        array_to_network(vec);

        send_safe(client_socket, (char *)&req, sizeof(Header));
        send_safe(client_socket, (char *)&cfg, sizeof(Config));
        send_safe(client_socket, (char *)matrix.data(), n * n * sizeof(int));
        send_safe(client_socket, (char *)vec.data(), n * sizeof(int));

        recv_safe(client_socket, (char *)&res, sizeof(Header));
        header_to_host(res);
        if (res.cmd == ACK_DATA_OK)
            cout << "Сервер прийняв конфігурацію і дані." << endl;

        cout << "Надсилання команди START..." << endl;
        req = {CMD_START, 0};
        header_to_network(req);
        send_safe(client_socket, (char *)&req, sizeof(Header));

        recv_safe(client_socket, (char *)&res, sizeof(Header));
        header_to_host(res);
        if (res.cmd == ACK_START_OK)
            cout << "Обчислення на сервері почалися." << endl;

        vector<int> result(n);
        bool is_done = false;

        while (!is_done)
        {
            Sleep(500);
            req = {CMD_STATUS, 0};
            header_to_network(req);
            send_safe(client_socket, (char *)&req, sizeof(Header));

            recv_safe(client_socket, (char *)&res, sizeof(Header));
            header_to_host(res);

            if (res.cmd == STATUS_WORKING)
            {
                cout << "Статус: Сервер ще рахує..." << endl;
            }
            else if (res.cmd == STATUS_DONE)
            {
                cout << "Статус: ГОТОВО! Отримання результату..." << endl;
                recv_safe(client_socket, (char *)result.data(), res.payload_size);
                array_to_host(result);
                is_done = true;
            }
        }

        cout << "\nПерші 5 елементів результуючого вектора: ";
        for (int i = 0; i < 5; ++i)
            cout << result[i] << " ";
        cout << "\nРоботу завершено успішно!" << endl;
    }
    catch (const exception &e)
    {
        cerr << "\n[ПОМИЛКА] " << e.what() << endl;
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}