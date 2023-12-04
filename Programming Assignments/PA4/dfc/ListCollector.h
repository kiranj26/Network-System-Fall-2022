//
// Created by young on 12/15/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_LISTCOLLECTOR_H
#define DISTRIBUTE_FILE_SYSTEM_LISTCOLLECTOR_H

#include <map>
#include <set>
#include <sys/epoll.h>

#include <iostream>

#include "conn.h"
#include "Conf.h"


class ListCollector;


class ListReader {
public:
    ListReader(int fd, ListCollector* collector) : m_fd(fd),
    m_collector(collector), m_state(0), m_remain_bytes(8), m_file_name_size(0) {}

    void readFilename();
private:
    int m_fd;
    int m_state;
    std::string m_file_name;
    ssize_t m_file_name_size;
    ssize_t m_remain_bytes;
    ListCollector* m_collector;
};


class ListCollector {
public:
    ListCollector(int fd1, int fd2, int fd3, int fd4) {
        m_done[fd1] = false;
        m_done[fd2] = false;
        m_done[fd3] = false;
        m_done[fd4] = false;

        file_sets[fd1] = std::set<std::string>();
        file_sets[fd2] = std::set<std::string>();
        file_sets[fd3] = std::set<std::string>();
        file_sets[fd4] = std::set<std::string>();


        readers[0] = new ListReader(fd1, this);
        readers[1] = new ListReader(fd2, this);
        readers[2] = new ListReader(fd3, this);
        readers[3] = new ListReader(fd4, this);

        m_epoll_fd = epoll_create1(0);
//        std::cout << "[ListCollector::ListCollector] m_epoll_fd : " << m_epoll_fd << std::endl;
        struct epoll_event event{};
        event.events = EPOLLIN;

        event.data.ptr = reinterpret_cast<void*>(readers[0]);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd1, &event);

        event.data.ptr = reinterpret_cast<void*>(readers[1]);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd2, &event);

        event.data.ptr = reinterpret_cast<void*>(readers[2]);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd3, &event);

        event.data.ptr = reinterpret_cast<void*>(readers[3]);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd4, &event);
    }

    virtual ~ListCollector()
    {
        for (auto & reader : readers)
        {
            delete reader;
        }

        close(m_epoll_fd);
    }

    ListReader& getReader(int i) {
        return *readers[i];
    }

    void addFile(int fd, std::string& file_name)
    {
//        std::cout << "[ListCollector::addFile] file added: " << file_name << std::endl;
        file_sets[fd].insert(file_name);
    }

    void finished(int fd)
    {
//        std::cout << "[ListCollector::finished] fd: " << fd << " is finished." << std::endl;
        m_done[fd] = true;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

        // if all m_done
        if (isDone())
        {
            std::map<std::string, std::set<int>> file_lists;

            for (auto const& [key, val]: file_sets)
            {
                for (auto const& file : val)
                {
//                    std::cout << "[ListCollector::finished] fd: " << key << ", " << file << std::endl;
                    size_t pos = file.find_last_of('.');
                    std::string original_file_name = file.substr(0, pos);
                    int part = std::stoi(file.substr(pos + 1));
                    file_lists[original_file_name].insert(part);
                }
            }

            std::set<int> all;
            all.insert(1);
            all.insert(2);
            all.insert(3);
            all.insert(4);

            for (auto const& [key, val]: file_lists)
            {
                if (val == all)
                {
                    std::cout << key << std::endl;
                }
                else
                {
                    std::cout << key << "\t[incomplete]" << std::endl;
                }
            }
        }
    }

    bool isDone()
    {
        for (auto const& [key, val] : m_done)
        {
//            std::cout << "[ListCollector::isDone] key, value: " << key << ", " << val << std::endl;
            if (!val)
                return false;
        }

        return true;
    }

    [[nodiscard]] int getEpollFD() const { return m_epoll_fd; }
private:
    int m_epoll_fd;
    std::map<int, bool> m_done;
    std::map<int, std::set<std::string>> file_sets;
    ListReader* readers[4]{};
};


#endif //DISTRIBUTE_FILE_SYSTEM_LISTCOLLECTOR_H
