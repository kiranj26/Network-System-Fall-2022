//
// Created by young on 12/14/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_CONNECTION_H
#define DISTRIBUTE_FILE_SYSTEM_CONNECTION_H


#include "EventLoop.h"
#include <map>
#include <iostream>
#include <unistd.h>  // for close

enum State
{
    INIT,
    HEADER_SIZE,
    HEADER
};


enum Command
{
    LIST = 0,
    GET = 1,
    PUT = 2,
    MKDIR = 3
};


class Connection : public Handler {
public:
    Connection(int conn_fd, std::string dir, std::map<std::string, std::string>& users) : m_conn_fd(conn_fd),
    m_state(INIT), m_header_size(0), m_remain_bytes(8),
    m_buf(nullptr), m_command(LIST), m_dir(std::move(dir)),
    m_file_fd(-1), m_users(users) {
        std::cout << "sizeof(m_command): " << sizeof(m_command) << std::endl;
    }
    ~Connection() override { close(m_conn_fd); }
    bool handle(EventLoop* loop) override;

    [[nodiscard]] int getFD() const override { return m_conn_fd; }
private:
    bool processHeader();
    int m_conn_fd;
    int m_state;
    ssize_t m_header_size;
    ssize_t m_remain_bytes;
    char* m_buf;
    Command m_command;
    std::string m_dir;
    std::string m_subdir;
    int m_file_fd;
    std::map<std::string, std::string>& m_users;
};


#endif //DISTRIBUTE_FILE_SYSTEM_CONNECTION_H
