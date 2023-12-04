//
// Created by young on 12/16/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_LISTREQUEST_H
#define DISTRIBUTE_FILE_SYSTEM_LISTREQUEST_H

#include <vector>

#include "Conf.h"
#include "conn.h"


class ListParser {
public:
    ListParser(int fd, std::vector<std::string>& parts) : m_fd(fd), m_state(0),
    m_remain_bytes(8), m_file_name_size(0), m_file_parts(parts), m_is_done(false) {}
    virtual ~ListParser() { close(m_fd); }
    bool readFilename();
    [[nodiscard]] bool isDone() const { return m_is_done; }
    [[nodiscard]] int getFD() const { return m_fd; }
private:
    int m_fd;
    int m_state;
    std::string m_file_name;
    ssize_t m_file_name_size;
    ssize_t m_remain_bytes;
    std::vector<std::string>& m_file_parts;
    bool m_is_done;
};


class ListRequest {
public:
    ListRequest(Conf& conf, std::string& subdir) : m_conf(conf), m_subdir(subdir), m_epoll_fd(-1) {}
    void list();
    bool allParserFinished();
private:
    std::pair<ssize_t, char*> listHeader();
    Conf m_conf;
    std::string m_subdir;
    int m_epoll_fd;
    std::vector<ListParser*> m_parsers;
    std::vector<std::string> m_file_parts;
};


#endif //DISTRIBUTE_FILE_SYSTEM_LISTREQUEST_H
