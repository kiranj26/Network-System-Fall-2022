//
// Created by young on 12/15/21.
//

#include "MultipleGetRequest.h"

#include <cstring>



std::pair<bool, std::string> MultipleGetRequest::requestFile(std::string remote_file, std::string subdir)
{
    m_epoll_fd = epoll_create1(0);

    ssize_t wrt_size;
    char* buf;
    std::tie(wrt_size, buf) = getHeader(remote_file, subdir);

    struct epoll_event event{};
    event.events = EPOLLIN;

    for (auto& address : m_conf.addresses)
    {
        int fd = conn(address.first, address.second);
        //            std::cout << "[MultipleGetRequest::requestFile] fd: " << fd << std::endl;
        if (fd < 0)
            continue;

        write(fd, buf, wrt_size);
        auto* fr = new FileReader(fd, this);
        m_readers.push_back(fr);
        event.data.ptr = reinterpret_cast<void*>(fr);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    }

    struct epoll_event events[4];

    // continue until all failed or at least one got it
    // note that a server may get a SIGPIPE signals if you close the socket here
    while (!allReaderFinished() && !someDone())
    {
        int count = epoll_wait(m_epoll_fd, events, sizeof(events), 100);
        for (int i = 0; i < count; i++) {
            auto *reader = reinterpret_cast<FileReader*>(events[i].data.ptr);
            if (reader->readFile())
            {
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, reader->getFD(), nullptr);
            }
        }
    }

    for (auto fr : m_readers)
    {
        close(fr->getFD());
    }

    close(m_epoll_fd);

    for (auto fr : m_readers)
    {
        // one of successful result
        if (fr->isDone())
        return std::pair(true, fr->getContent());
    }

    return std::pair(false, "");
}


bool MultipleGetRequest::someDone()
{
    for (auto fr : m_readers)
        if (fr->isDone())
            return true;
    return false;
}

bool MultipleGetRequest::allReaderFinished()
{
    for (auto fr : m_readers)
    {
        if (!fr->isFinished())
            return false;
    }

    return true;
}


std::pair<ssize_t, char*> MultipleGetRequest::getHeader(std::string& part_name, std::string& subdir) const
{
    size_t header_size = 4;
    header_size += 8 + m_conf.m_user.length();
    header_size += 8 + m_conf.m_password.length();
    header_size += 8 + subdir.length();
    header_size += 8 + part_name.length();

    char* msg = (char*)malloc(header_size + 8);
    char* msg_pos = msg;

    ///////////////////////////////////////////////////////////////////////////
    // header_size (8) + command (4)
    memcpy(msg_pos, &header_size, sizeof(header_size));
    msg_pos += sizeof(header_size);

    ///////////////////////////////////////////////////////////////////////////
    // get method!
    int command = 1;
    memcpy(msg_pos, &command, sizeof(command));
    msg_pos += sizeof(command);

    ///////////////////////////////////////////////////////////////////////////
    // user
    size_t temp = m_conf.m_user.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_user.c_str(), m_conf.m_user.length());
    msg_pos += m_conf.m_user.length();

    ///////////////////////////////////////////////////////////////////////////
    // password
    temp = m_conf.m_password.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_password.c_str(), m_conf.m_password.length());
    msg_pos += m_conf.m_password.length();

    ///////////////////////////////////////////////////////////////////////////
    // subdir
    temp = subdir.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)subdir.c_str(), subdir.length());
    msg_pos += subdir.length();

    ///////////////////////////////////////////////////////////////////////////
    // file name
    temp = part_name.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)part_name.c_str(), part_name.length());
    msg_pos += part_name.length();

    return std::pair<int, char*>(header_size + 8, msg);
}