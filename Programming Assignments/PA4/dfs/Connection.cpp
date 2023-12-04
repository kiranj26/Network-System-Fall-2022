//
// Created by young on 12/14/21.
//
#include <unistd.h>
#include <iostream>
#include <filesystem>

#include <fcntl.h>     // for open
#include <cstring>     // for memcpy
#include <sys/stat.h>  // for fstat

#include "Connection.h"


namespace fs = std::filesystem;


bool Connection::handle(EventLoop* loop)
{
    if (m_state == INIT)
    {
        ///////////////////////////////////////////////////////////////////////
        //        takes a `header_len` field (8 bytes, ssize_t type)         //
        ///////////////////////////////////////////////////////////////////////
        ssize_t n = read(m_conn_fd,
                         reinterpret_cast<char*>(&m_header_size) + (sizeof(m_header_size) - m_remain_bytes),
                         m_remain_bytes);

        if (n <= 0)
        {
            // note that Connection does not own m_fd
            // m_fd will be closed in the event loop
            return true;
        }

        m_remain_bytes -= n;
        if (m_remain_bytes == 0)
        {
            std::cout << "[Connection::handle] header_size: " << m_header_size << std::endl;
            m_state = HEADER_SIZE;
            m_buf = (char*)malloc(m_header_size);
            m_remain_bytes = m_header_size;
            return false;
        }
    }
    else if (m_state == HEADER_SIZE)
    {
        ///////////////////////////////////////////////////////////////////////
        //            takes a header (`m_header_size` bytes)                 //
        ///////////////////////////////////////////////////////////////////////
        ssize_t n = read(m_conn_fd, m_buf + (m_header_size - m_remain_bytes), m_remain_bytes);

        if (n <= 0)
        {
            return true;
        }

        m_remain_bytes -= n;
        if (m_remain_bytes == 0)
        {
            // complete header is received!
            m_state = HEADER;
            return processHeader();
        }
    }
    else if (m_state == HEADER)
    {
        ///////////////////////////////////////////////////////////////////////
        // only PUT command has BODY
        // takes BODY and writes to a file
        ///////////////////////////////////////////////////////////////////////
        static char buf[1024];
        ssize_t n = read(m_conn_fd, buf, m_remain_bytes > sizeof(buf) ? sizeof(buf) : m_remain_bytes);

        if (n <= 0)
        {
            return true;
        }

        write(m_file_fd, buf, n);
        m_remain_bytes -= n;

        if (m_remain_bytes == 0)
        {
            close(m_file_fd);

            // OK ret
            ssize_t t = 0;
            write(m_conn_fd, &t, sizeof(t));

            m_state = INIT;
            m_remain_bytes = 8;
        }

        return false;
    }

    return false;
}


/**
 * a complete header is received
 * do some action (authentication, ...)
 * don't forget free `m_buf`
 *
 * @return
 */
bool Connection::processHeader()
{
    char* buf_pos = m_buf;

    memcpy(&m_command, buf_pos, sizeof(m_command));
    buf_pos += sizeof(m_command);

    std::cout << "[Connection::processHeader] command: " << m_command << std::endl;

    ///////////////////////////////////////////////////////////////////////////
    //                            user field                                 //
    ///////////////////////////////////////////////////////////////////////////
    ssize_t user_len;
    memcpy(&user_len, buf_pos, sizeof(ssize_t));
    buf_pos += sizeof(ssize_t);
    std::string user = std::string(buf_pos, user_len);
    std::cout << "[Connection::processHeader] user: " << user << std::endl;
    buf_pos += user_len;

    ///////////////////////////////////////////////////////////////////////////
    //                            password field                             //
    ///////////////////////////////////////////////////////////////////////////
    ssize_t password_len;
    memcpy(&password_len, buf_pos, sizeof(ssize_t));
    buf_pos += sizeof(ssize_t);
    std::string password = std::string(buf_pos, password_len);
    std::cout << "[Connection::processHeader] password: " << password << std::endl;
    buf_pos += password_len;

    // if user and passwd match?
    // if not it terminates the connection!
    if (m_users.find(user) == m_users.end() || m_users[user] != password)
    {
        std::cout << "invalid Username/Password. Please try again." << std::endl;
        ssize_t ret = -2;
        write(m_conn_fd, &ret, sizeof(ret));
        free(m_buf);
        return true;
    }

    // authentication is m_done!
    // make sure that there is a user directory!
    auto user_dir = fs::path(m_dir) / fs::path(user);
    if (fs::exists(user_dir))
    {
        std::cout << "[Connection::processHeader] user directory exists." << std::endl;
    }
    else
    {
        fs::create_directories(user_dir);
    }

    ///////////////////////////////////////////////////////////////////////////
    //                            subdir field                               //
    ///////////////////////////////////////////////////////////////////////////
    ssize_t subdir_len;
    memcpy(&subdir_len, buf_pos, sizeof(ssize_t));
    buf_pos += sizeof(ssize_t);

    m_subdir = std::string(buf_pos, subdir_len);
    buf_pos += subdir_len;
    std::cout << "[Connection::processHeader] subdir: " << m_subdir << std::endl;

    if (m_command == LIST)
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //                        LIST commands                              //
        //
        ///////////////////////////////////////////////////////////////////////
        fs::path dir_path = fs::path(m_dir) / fs::path(user) / fs::path(m_subdir);
        size_t file_name_len;

        if (fs::exists(dir_path))
        {
            for (const auto &entry: std::filesystem::directory_iterator(dir_path)) {
                if (!fs::is_regular_file(entry))
                    continue;
                std::string file_name = entry.path().filename().string().substr(1);
                file_name_len = file_name.length();
                write(m_conn_fd, &file_name_len, sizeof(file_name_len));
                write(m_conn_fd, file_name.c_str(), file_name_len);
            }
        }
        file_name_len = 0;
        write(m_conn_fd, &file_name_len, sizeof(file_name_len));

        free(m_buf);
        m_state = INIT;
        m_remain_bytes = 8;
    }
    else if (m_command == GET)
    {
        ///////////////////////////////////////////////////////////////////////
        //                        GET command                                //
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // additional file name field
        ///////////////////////////////////////////////////////////////////////
        ssize_t file_name_len;
        memcpy(&file_name_len, buf_pos, sizeof(ssize_t));
        buf_pos += sizeof(ssize_t);

        std::string file_name = "." + std::string(buf_pos, file_name_len);
        std::cout << "[Connection::processHeader] GET, file name: " << file_name << std::endl;

        fs::path file_path = fs::path(m_dir) / fs::path(user) / fs::path(m_subdir) / fs::path(file_name);
        if (!fs::exists(file_path))
        {
            // -1 indicates the absence of the file!
            ssize_t ret = -1;
            write(m_conn_fd, &ret, sizeof(ret));
        }
        else
        {
            int file_fd = open(file_path.string().c_str(), O_RDONLY);

            struct stat file_stat{};
            fstat(file_fd, &file_stat);

            std::cout << "[Connection::processHeader] file size: " << file_stat.st_size << std::endl;
            ssize_t m = write(m_conn_fd, &(file_stat.st_size), sizeof(file_stat.st_size));
            std::cout << "[Connection::processHeader] m bytes written: " << m << std::endl;
            if (m <= 0)
            {
                close(m_conn_fd);
                close(file_fd);
                std::cout << "[Connection::processHeader] write m: " << m << std::endl;
            }
            else {
                static char buf[1024];

                while (true) {
                    ssize_t n = read(file_fd, buf, sizeof(buf));
                    std::cout << "[Connection::processHeader] read from local file n: " << n << std::endl;
                    if (n <= 0) {
                        close(file_fd);
                        break;
                    }
                    m = write(m_conn_fd, buf, n);
                    std::cout << "[Connection::processHeader] wrote to socket m: " << m << std::endl;
                    if (m <= 0) {
                        close(m_conn_fd);
                        std::cout << "[Connection::processHeader] write m: " << m << std::endl;
                        break;
                    }
                }
            }
        }

        free(m_buf);
        m_state = INIT;
        m_remain_bytes = 8;
    }
    else if (m_command == PUT)
    {
        ///////////////////////////////////////////////////////////////////////
        // m_command == PUT!
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // additionally,
        // file_name field
        ssize_t file_name_len;
        memcpy(&file_name_len, buf_pos, sizeof(ssize_t));
        buf_pos += sizeof(ssize_t);

        std::string file_name = "." + std::string(buf_pos, file_name_len);
        std::cout << "[Connection::processHeader] PUT, file name: " << file_name << std::endl;
        buf_pos += file_name_len;

        ///////////////////////////////////////////////////////////////////////
        // additionally,
        // file size field -> m_remain_bytes
        memcpy(&m_remain_bytes, buf_pos, sizeof(m_remain_bytes));
        buf_pos += sizeof(m_remain_bytes);
        std::cout << "[Connection::processHeader] bytes to read: " << m_remain_bytes << std::endl;

        // file is created!
        fs::path file_path = fs::path(m_dir) / fs::path(user) / fs::path(m_subdir) / fs::path(file_name);
        m_file_fd = open(file_path.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    }
    else if (m_command == MKDIR)
    {
        ///////////////////////////////////////////////////////////////////////
        //                  m_command = MKDIR                                //
        ///////////////////////////////////////////////////////////////////////
        fs::path dir_path = fs::path(m_dir) / fs::path(user) / fs::path(m_subdir);
        if (fs::exists(dir_path))
        {
            std::cout << "[Connection::processHeader] user directory exists." << std::endl;
        }
        else
        {
            fs::create_directories(dir_path);
        }
        ssize_t t = 0;
        write(m_conn_fd, &t, sizeof(t));

        free(m_buf);
    }

    return false;
}