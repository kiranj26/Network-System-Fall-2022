//
// Created by young on 12/14/21.
//

#include <netinet/in.h>
#include "DFS.h"
#include "Connection.h"

#include <iostream>
#include <cerrno>
#include <cstring>


DFS::DFS(std::string dir, int port, std::map<std::string, std::string>& users) : m_dir(std::move(dir)),
m_users(users), m_port(port)
{
    sockaddr_in serv_addr{};
    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));

    // assign IP, PORT
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    // Binding newly created socket to given IP and verification
    if ((bind(m_sock_fd, (sockaddr*)&serv_addr, sizeof(serv_addr))) != 0) {
        throw SocketException(std::string("socket bind failed...\n"));
    }

    // Now server is ready to listen and verification
    if ((listen(m_sock_fd, 5)) != 0) {
        throw SocketException(std::string("Listen failed...\n"));
    }
}

/**
 *
 * @param loop
 * @return
 */
bool DFS::handle(EventLoop* loop)
{
    sockaddr_in cli{};
    socklen_t len = sizeof(cli);

    try {
        int conn_fd = accept(m_sock_fd, (sockaddr*)&cli, &len);
        if (conn_fd < 0) {
            std::cerr << "accept error: " << strerror(errno) << std::endl;
        } else {
            // new connection "conn_fd" is handled with a new HttpHandler instance
            auto* handler = new Connection(conn_fd, m_dir, m_users);
            loop->addHandler(handler);
            std::cout << "new connection is established" << ", fd: " << conn_fd << std::endl;
        }
    } catch (std::invalid_argument& ia) {
        std::cerr << "in Server::handler" << std::endl;
        std::cerr << ia.what() << std::endl;
    }

    // Server never stops! It keeps accepting connections.
    return false;
}