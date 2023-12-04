//
// Created by young on 12/15/21.
//

#include <unistd.h>
#include "ListCollector.h"


void ListReader::readFilename()
{
//    std::cout << "[ListReader::readFilename] m_state: " << m_state << std::endl;
    if (m_state == 0)
    {
        ssize_t n = read(m_fd,
                         &m_file_name_size + (sizeof(ssize_t) - m_remain_bytes),
                         m_remain_bytes);
//        std::cout << "[ListReader::readFilename] (file_name_size) fd: " << m_fd << ", bytes read: " << n << ", n <= 0: " << (n <= 0) << std::endl;

        if (n <= 0) {
            m_collector->finished(m_fd);
            return;
        }

        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
//            std::cout << "[ListReader::readFilename] file name size: " << m_file_name_size << std::endl;
            if (m_file_name_size == 0)
            {
                // it's the end of response
                // unregister m_dfs1 from epoll_df
                m_collector->finished(m_fd);
                return;
            }
            else
            {
                // needs to retrieve new file item (`file_name_size1` bytes long)
                m_state = 1;
                m_remain_bytes = m_file_name_size;
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
            m_collector->finished(m_fd);
            return;
        }

        m_file_name += std::string(buf, n);
        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
            // insert file_name1 into some set!
            m_collector->addFile(m_fd, m_file_name);
            m_file_name.erase();

            m_state = 0;
            m_remain_bytes = 8;
        }
    }
}