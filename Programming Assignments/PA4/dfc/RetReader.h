//
// Created by young on 12/16/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_RETREADER_H
#define DISTRIBUTE_FILE_SYSTEM_RETREADER_H

#include <unistd.h>


class RetReader {
public:
    explicit RetReader(int fd) : m_fd(fd), m_remain_bytes(8), m_is_done(false), m_ret(-1) {}
    virtual ~RetReader() { close(m_fd); }
    bool readRet()
    {
        ssize_t n = read(m_fd, &m_ret + (sizeof(m_ret) - m_remain_bytes), m_remain_bytes);
        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
            m_is_done = true;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool isDone() const { return m_is_done; }
    [[nodiscard]] int getFD() const { return m_fd; }
    [[nodiscard]] ssize_t getRet() const { return m_ret; }
private:
    int m_fd;
    ssize_t m_ret;
    ssize_t m_remain_bytes;
    bool m_is_done;
};


#endif //DISTRIBUTE_FILE_SYSTEM_RETREADER_H
