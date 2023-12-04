//
// Created by young on 12/15/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_MULTIPLEGETREQUEST_H
#define DISTRIBUTE_FILE_SYSTEM_MULTIPLEGETREQUEST_H


#include <string>
#include <tuple>
#include <utility>

#include <unistd.h>

#include <vector>
#include <iostream>
#include <fstream>

#include <sys/epoll.h>

#include "Conf.h"
#include "conn.h"


class MultipleGetRequest;


class FileReader
{
public:
    FileReader(int fd, MultipleGetRequest* request) : m_fd(fd), m_state(0), m_byte_read(8), m_done(false),
    m_file_size(-1), m_request(request), m_finished(false) {}

    bool readFile()
    {
        if (m_state == 0)
        {
            ssize_t n = read(m_fd, &m_file_size + (sizeof(ssize_t) - m_byte_read), m_byte_read);

            if (n <= 0)
            {
                m_finished = true;
                return true;
            }

            m_byte_read -= n;

            if (m_byte_read == 0)
            {
                if (m_file_size < 0)
                {
                    if (m_file_size == -2)
                    {
                        std::cout << "Invalid Username/Password. Please try again." << std::endl;
                    }
                    m_finished = true;
                    return true;
                }
                m_state = 1;
                m_byte_read = m_file_size;
            }
        }
        else if (m_state == 1)
        {
            static char buf[1024];
            ssize_t n = read(m_fd, buf, m_byte_read > sizeof(buf) ? sizeof(buf) : m_byte_read);

            if (n <= 0)
            {
                m_finished = true;
                return true;
            }

            // decrypt
            for (int i = 0; i < n; i++)
                buf[i] ^= 77;
            content.append(std::string(buf, n));
            m_byte_read -= n;

            if (m_byte_read == 0)
            {
                m_finished = true;
                // finally, success!
                m_done = true;
                return true;
            }
        }

        return false;
    }
    [[nodiscard]] bool isFinished() const { return m_finished; }
    [[nodiscard]] bool isDone() const { return m_done; }
    [[nodiscard]] int getFD() const { return m_fd; }
    std::string getContent() { return content; }
private:
    int m_state;
    ssize_t m_byte_read;
    ssize_t m_file_size;
    int m_fd;
    std::string content;
    bool m_done;
    bool m_finished;
    MultipleGetRequest* m_request;
};


class MultipleGetRequest {
public:
    explicit MultipleGetRequest(Conf& conf) : m_conf(conf), m_done(false), m_epoll_fd(-1) {}

    std::pair<bool, std::string> requestFile(std::string remote_file, std::string subdir);

    bool someDone();
//    {
//        for (auto fr : m_readers)
//            if (fr->isDone())
//                return true;
//        return false;
//    }

    bool allReaderFinished();
//    {
//        for (auto fr : m_readers)
//        {
//            if (!fr->isFinished())
//                return false;
//        }
//
//        return true;
//    }

private:
    std::pair<ssize_t, char*> getHeader(std::string& part_name, std::string& subdir) const;
    Conf m_conf;
    int m_epoll_fd;
    bool m_done;
    std::vector<FileReader*> m_readers;
};


#endif //DISTRIBUTE_FILE_SYSTEM_MULTIPLEGETREQUEST_H
