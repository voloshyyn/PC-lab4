#pragma once
#include <vector>
#include <winsock2.h>

enum Command : int
{
    CMD_SEND_DATA = 1,
    CMD_START = 2,
    CMD_STATUS = 3,
    ACK_DATA_OK = 101,
    ACK_START_OK = 102,
    STATUS_WORKING = 103,
    STATUS_DONE = 104
};

struct Header
{
    Command cmd;
    int payload_size;
};

struct Config
{
    int n;
    int num_threads;
};

inline void header_to_network(Header &h)
{
    h.cmd = static_cast<Command>(htonl(static_cast<u_long>(h.cmd)));
    h.payload_size = htonl(static_cast<u_long>(h.payload_size));
}

inline void header_to_host(Header &h)
{
    h.cmd = static_cast<Command>(ntohl(static_cast<u_long>(h.cmd)));
    h.payload_size = ntohl(static_cast<u_long>(h.payload_size));
}

inline void config_to_network(Config &c)
{
    c.n = htonl(static_cast<u_long>(c.n));
    c.num_threads = htonl(static_cast<u_long>(c.num_threads));
}

inline void config_to_host(Config &c)
{
    c.n = ntohl(static_cast<u_long>(c.n));
    c.num_threads = ntohl(static_cast<u_long>(c.num_threads));
}

inline void array_to_network(std::vector<int> &arr)
{
    for (int &val : arr)
        val = htonl(static_cast<u_long>(val));
}

inline void array_to_host(std::vector<int> &arr)
{
    for (int &val : arr)
        val = ntohl(static_cast<u_long>(val));
}