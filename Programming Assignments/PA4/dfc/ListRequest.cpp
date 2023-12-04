//
// Created by young on 12/16/21.
//

#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <tuple>
#include <set>
#include <map>

#include "ListRequest.h"



bool ListParser::readFilename()
{
//    std::cout << "[ListReader::readFilename] m_state: " << m_state << std::endl;
    if (m_state == 0)
    {
        ssize_t n = read(m_fd,
                         &m_file_name_size + (sizeof(ssize_t) - m_remain_bytes),
                         m_remain_bytes);
//        std::cout << "[ListReader::readFilename] (file_name_size) fd: " << m_fd << ", bytes read: " << n << ", n <= 0: " << (n <= 0) << std::endl;

        if (n <= 0) {
            m_is_done = true;
            return true;
        }

        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
//            std::cout << "[ListReader::readFilename] file name size: " << m_file_name_size << std::endl;
            if (m_file_name_size == 0)
            {
                // it's the end of response
                // unregister m_dfs1 from epoll_df
                m_is_done = true;
                return true;
            }
            else if (m_file_name_size == -2)
            {
                std::cout << "Invalid Username/Password. Please try again." << std::endl;
                m_is_done = true;
                return true;
            }
            else
            {
                // needs to retrieve new file item (`m_file_name_size` bytes long)
                m_state = 1;
                m_remain_bytes = m_file_name_size;
                return false;
            }
        }
    }
    else
    {
        // here state == 1 !
        static char buf[1024];

        ssize_t n = read(m_fd, buf, sizeof(buf) > m_remain_bytes ? m_remain_bytes : sizeof(buf));
//        std::cout << "[ListReader::readFilename] (file_name) bytes read: " << n << std::endl;

        if (n <= 0)
        {
            m_is_done = true;
            return true;
        }

        m_file_name += std::string(buf, n);
        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
            // insert file_name into some set!
            m_file_parts.push_back(m_file_name);
            m_file_name.erase();

            m_state = 0;
            m_remain_bytes = 8;
        }
    }

    return false;
}


bool ListRequest::allParserFinished()
{
    for (auto& parser : m_parsers)
    {
        if (!parser->isDone())
            return false;
    }

    return true;
}


void ListRequest::list()
{
    m_epoll_fd = epoll_create1(0);
    struct epoll_event event{};
    event.events = EPOLLIN;

    ssize_t wrt_size;
    char* buf;
    std::tie(wrt_size, buf) = listHeader();

    for (auto& address : m_conf.addresses)
    {
        int fd = conn(address.first, address.second);
//        std::cout << "[ListRequest::list] new connection fd: " << fd << std::endl;
        if (fd > 0)
        {
            write(fd, buf, wrt_size);
            auto* parser = new ListParser(fd, m_file_parts);
            m_parsers.push_back(parser);
            event.data.ptr = reinterpret_cast<void*>(parser);
            epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
        }
    }

    free(buf);

    struct epoll_event events[4];

    while (!allParserFinished())
    {
        int count = epoll_wait(m_epoll_fd, events, sizeof(events), 100);
//        std::cout << "[ListRequest::list] readable event: " << count << std::endl;
        for (int i = 0; i < count; i++) {
            auto *reader = reinterpret_cast<ListParser*>(events[i].data.ptr);
            if (reader->readFilename())
            {
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, reader->getFD(), nullptr);
            }
        }
    }

    // when it's done.
    std::map<std::string, std::set<int>> parts_for_file;
    for (auto& s : m_file_parts)
    {
//        std::cout << "file_parts: " << s << std::endl;
//        size_t len = s.length();

        size_t pos = s.find_last_of('.');
//        std::cout << "pos : " << pos << std::endl;

        if (pos < s.length()) {
            std::string original_file_name = s.substr(0, pos);
            int part = std::stoi(s.substr(pos + 1));
            parts_for_file[s.substr(0, pos)].insert(part);
        }
    }

    std::set<int> complete_sets({1, 2, 3, 4});

    for (auto const& [key, val]: parts_for_file)
    {
        if (val == complete_sets)
        {
            std::cout << key << std::endl;
        }
        else
        {
            std::cout << key << "\t[incomplete]" << std::endl;
        }
    }
}


std::pair<ssize_t, char*> ListRequest::listHeader()
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

    int command = 0;
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