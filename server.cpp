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

class ClientSession
{
private:
    SOCKET client_socket;
    atomic<int> calc_status;
    vector<int> matrix;
    vector<int> vec;
    vector<int> result;
    Config config;

    void multiply_worker(int n, int start_row, int end_row)
    {
        try
        {
            for (int i = start_row; i < end_row; ++i)
            {
                result[i] = 0;
                for (int j = 0; j < n; ++j)
                {
                    result[i] += matrix[i * n + j] * vec[j];
                }
            }
        }
        catch (...)
        {
            calc_status = -1; // Позначаємо помилку
        }
    }

    void background_computation()
    {
        int n = config.n;
        int threads_count = config.num_threads;

        vector<thread> workers;
        int chunk_size = n / threads_count;
        int remainder = n % threads_count;
        int current_row = 0;

        for (int i = 0; i < threads_count; ++i)
        {
            int end_row = current_row + chunk_size + (i < remainder ? 1 : 0);
            workers.emplace_back(&ClientSession::multiply_worker, this, n, current_row, end_row);
            current_row = end_row;
        }

        for (auto &t : workers)
        {
            if (t.joinable())
                t.join();
        }

        if (calc_status != -1)
            calc_status = 2;
    }

public:
    ClientSession(SOCKET s) : client_socket(s), calc_status(0) {}

    ~ClientSession()
    {
        closesocket(client_socket);
        cout << "[Сервер] Сесію завершено, сокет закрито." << endl;
    }

    void run()
    {
        try
        {
            while (true)
            {
                Header req_header;
                recv_safe(client_socket, (char *)&req_header, sizeof(Header));
                header_to_host(req_header);

                Header res_header;

                switch (req_header.cmd)
                {
                case CMD_SEND_DATA:
                {
                    recv_safe(client_socket, (char *)&config, sizeof(Config));
                    config_to_host(config);

                    int n = config.n;
                    if (n <= 0 || n > 10000)
                        throw runtime_error("Невірний розмір матриці.");

                    matrix.resize(n * n);
                    vec.resize(n);
                    result.resize(n);

                    recv_safe(client_socket, (char *)matrix.data(), n * n * sizeof(int));
                    recv_safe(client_socket, (char *)vec.data(), n * sizeof(int));

                    array_to_host(matrix);
                    array_to_host(vec);

                    calc_status = 0;
                    res_header = {ACK_DATA_OK, 0};
                    header_to_network(res_header);
                    send_safe(client_socket, (char *)&res_header, sizeof(Header));
                    break;
                }
                case CMD_START:
                {
                    if (calc_status == 0)
                    {
                        calc_status = 1;
                        thread(&ClientSession::background_computation, this).detach();
                    }
                    res_header = {ACK_START_OK, 0};
                    header_to_network(res_header);
                    send_safe(client_socket, (char *)&res_header, sizeof(Header));
                    break;
                }
                case CMD_STATUS:
                {
                    if (calc_status == 1)
                    {
                        res_header = {STATUS_WORKING, 0};
                        header_to_network(res_header);
                        send_safe(client_socket, (char *)&res_header, sizeof(Header));
                    }
                    else if (calc_status == 2)
                    {
                        int result_size = config.n * sizeof(int);
                        res_header = {STATUS_DONE, result_size};
                        header_to_network(res_header);
                        send_safe(client_socket, (char *)&res_header, sizeof(Header));

                        vector<int> result_net = result;
                        array_to_network(result_net);
                        send_safe(client_socket, (char *)result_net.data(), result_size);
                        calc_status = 0;
                    }
                    else if (calc_status == -1)
                    {
                        throw runtime_error("Помилка обчислень.");
                    }
                    break;
                }
                default:
                    throw runtime_error("Невідома команда.");
                }
            }
        }
        catch (const exception &e)
        {
            cerr << "[Помилка сесії] " << e.what() << endl;
        }
    }
};
