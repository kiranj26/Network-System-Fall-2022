//
// Created by young on 12/16/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_MKDIRREQUEST_H
#define DISTRIBUTE_FILE_SYSTEM_MKDIRREQUEST_H

#include <string>
#include <vector>

#include "Conf.h"
#include "conn.h"
#include "RetReader.h"


class MkdirRequest {
public:
    MkdirRequest(Conf& conf, std::string& subdir) : m_conf(conf), m_subdir(subdir) {}
    void mkdir();
    bool allReaderFinished()
    {
        for (auto& reader : m_readers)
        {
            if (!reader->isDone())
                return false;
        }
        return true;
    }
private:
    std::pair<ssize_t, char*> mkdirHeader();
    Conf m_conf;
    std::string m_subdir;
    std::vector<RetReader*> m_readers;
};


#endif //DISTRIBUTE_FILE_SYSTEM_MKDIRREQUEST_H
