//
// Created by young on 12/16/21.
//

#include <cstring>
#include <sys/epoll.h>
#include <tuple>

#include "MkdirRequest.h"



void MkdirRequest::mkdir()
{
    int epoll_fd = epoll_create1(0);

    ssize_t wrt_size;
    char* buf;
    std::tie(wrt_size, buf) = mkdirHeader();

    struct epoll_event event{};
    event.events = EPOLLIN;

    for (auto& address : m_conf.addresses)
    {
        int fd = conn(address.first, address.second);
        if (fd > 0)
        {
            write(fd, buf, wrt_size);
            auto* reader = new RetReader(fd);
            m_readers.push_back(reader);
            event.data.ptr = reinterpret_cast<void*>(reader);
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
        }
    }

    free(buf);

    struct epoll_event events[4];

    while (!allReaderFinished())
    {
        int count = epoll_wait(epoll_fd, events, sizeof(events), 100);
        for (int i = 0; i < count; i++) {
            auto *reader = reinterpret_cast<RetReader*>(events[i].data.ptr);
            if (reader->readRet())
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, reader->getFD(), nullptr);
            }
        }
    }

    for (auto& reader : m_readers)
    {
        std::cout << "[MkdirRequest::mkdir] ret: " << reader->getRet() << std::endl;
    }

    close(epoll_fd);
}

std::pair<ssize_t, char*> MkdirRequest::mkdirHeader()
{
    size_t header_size = 4;
    header_size += 8 + m_conf.m_user.length();
    header_size += 8 + m_conf.m_password.length();
    header_size += 8 + m_subdir.length();

//    std::cout << "[DFC::list] header_size: " << header_size << std::endl;
    char* msg = (char*)malloc(header_size + 8);
    char* msg_pos = msg;
    memcpy(msg_pos, &header_size, sizeof(header_size));
    msg_pos += sizeof(header_size);

    ///////////////////////////////////////////////////////////////////////////
    // MKDIR = 3!
    int command = 3;
    memcpy(msg_pos, &command, sizeof(command));
    msg_pos += sizeof(command);

    ///////////////////////////////////////////////////////////////////////////
    size_t temp = m_conf.m_user.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_user.c_str(), m_conf.m_user.length());
    msg_pos += m_conf.m_user.length();

    ///////////////////////////////////////////////////////////////////////////
    temp = m_conf.m_password.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_password.c_str(), m_conf.m_password.length());
    msg_pos += m_conf.m_password.length();

    ///////////////////////////////////////////////////////////////////////////
    temp = m_subdir.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_subdir.c_str(), m_subdir.length());
    msg_pos += m_subdir.length();

    return std::pair(header_size + 8, msg);
}