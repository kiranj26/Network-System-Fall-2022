//
// Created by young on 12/14/21.
//

#ifndef PA4_DFS_H
#define PA4_DFS_H

#include <string>
#include <map>

#include "EventLoop.h"


class SocketException: public std::exception {
public:
    explicit SocketException(std::string s) {
        msg = std::move(s);
    }
private:
    std::string msg;
};


class DFS : public Handler {
public:
    DFS(std::string dir, int port, std::map<std::string, std::string>& users);
    bool handle(EventLoop* loop) override;

    [[nodiscard]] int getFD() const override { return m_sock_fd; }
private:
    int m_sock_fd;
    std::string m_dir;
    int m_port;
    std::map<std::string, std::string>& m_users;
};


#endif //PA4_DFS_H
