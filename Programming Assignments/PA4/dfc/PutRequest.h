//
// Created by young on 12/16/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_PUTREQUEST_H
#define DISTRIBUTE_FILE_SYSTEM_PUTREQUEST_H

#include <string>
#include <utility>
#include <cstring>

#include "Conf.h"
#include "conn.h"


class PutRequest {
public:
    PutRequest(Conf& conf, std::string& local_file, std::string& subdir) : m_conf(conf),
    m_local_file(local_file), m_subdir(subdir) {}
    void put();
private:
    std::pair<ssize_t, char*> putHeader(int part, ssize_t part_size);
    Conf m_conf;
    std::string m_local_file;
    std::string m_subdir;
};


#endif //DISTRIBUTE_FILE_SYSTEM_PUTREQUEST_H
