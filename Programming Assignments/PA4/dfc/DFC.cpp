//
// Created by young on 12/15/21.
//

#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>  // for memcpy
#include <fcntl.h> // for open, O_RDONLY
#include <sys/stat.h> // for fstat

#include <filesystem>

#include <openssl/md5.h>

#include "DFC.h"
#include "ListCollector.h"

#include <iostream>


namespace fs = std::filesystem;

/**
 *
 * @param subdir
 *
 * ListCollector does most of works
 * ListCollector maintains 4 ListReaders and when all 4 are done, it knows that
 * the work is done!
 */
void DFC::list(std::string& subdir)
{
    ///////////////////////////////////////////////////////////////////////////
    // send list commands to all servers
    ///////////////////////////////////////////////////////////////////////////
    size_t header_size = 4 + 8 + m_user.length() + 8 + m_password.length() + 8 + subdir.length();
//    std::cout << "[DFC::list] header_size: " << header_size << std::endl;
    char* msg = (char*)malloc(header_size + 8);
    char* msg_pos = msg;
    memcpy(msg_pos, &header_size, sizeof(header_size));
    msg_pos += sizeof(header_size);

    int command = 0;
    memcpy(msg_pos, &command, sizeof(command));
    msg_pos += sizeof(command);

    ///////////////////////////////////////////////////////////////////////////
    size_t temp = m_user.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_user.c_str(), m_user.length());
    msg_pos += m_user.length();

    ///////////////////////////////////////////////////////////////////////////
    temp = m_password.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_password.c_str(), m_password.length());
    msg_pos += m_password.length();

    ///////////////////////////////////////////////////////////////////////////
    temp = subdir.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)subdir.c_str(), subdir.length());
    msg_pos += subdir.length();

    write(m_dfs1, msg, header_size + 8);
    write(m_dfs2, msg, header_size + 8);
    write(m_dfs3, msg, header_size + 8);
    write(m_dfs4, msg, header_size + 8);

    free(msg);

    ///////////////////////////////////////////////////////////////////////////
    // retrieve all responses
    ///////////////////////////////////////////////////////////////////////////
    ListCollector collector(m_dfs1, m_dfs2, m_dfs3, m_dfs4);

    struct epoll_event events[4];

    while (!collector.isDone())
    {
        int count = epoll_wait(collector.getEpollFD(), events, sizeof(events), 100);
        for (int i = 0; i < count; i++) {
//            std::cout << "[DFC::list] readable events" << std::endl;
            auto *reader = reinterpret_cast<ListReader*>(events[i].data.ptr);
            reader->readFilename();
        }
    }
}


std::pair<ssize_t, char*> DFC::put_header(std::string& part_name, ssize_t part_size, std::string& subdir)
{
    size_t header_size = 4;
    header_size += 8 + m_user.length();
    header_size += 8 + m_password.length();
    header_size += 8 + subdir.length();
    header_size += 8 + part_name.length();
    header_size += 8;

    char* msg = (char*)malloc(header_size + 8);
    char* msg_pos = msg;

    memcpy(msg_pos, &header_size, sizeof(header_size));
    msg_pos += sizeof(header_size);

    int command = 2;
    memcpy(msg_pos, &command, sizeof(command));
    msg_pos += sizeof(command);

    ///////////////////////////////////////////////////////////////////////////
    // user
    size_t temp = m_user.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_user.c_str(), m_user.length());
    msg_pos += m_user.length();

    ///////////////////////////////////////////////////////////////////////////
    // password
    temp = m_password.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_password.c_str(), m_password.length());
    msg_pos += m_password.length();

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


    ///////////////////////////////////////////////////////////////////////////
    // file size
    memcpy(msg_pos, &part_size, sizeof(part_size));
    msg_pos += sizeof(part_size);

    return std::pair<int, char*>(header_size + 8, msg);
}


void DFC::put(std::string& local_file, std::string& subdir)
{
    std::cout << fs::current_path() << std::endl;
    fs::path file_path = (fs::current_path() / fs::path(local_file));
    if (!fs::exists(file_path))
    {
        std::cout << "file not found." << std::endl;
        return;
    }

    int fd = open(file_path.string().c_str(), O_RDONLY);

    struct stat file_stat{};
    fstat(fd, &file_stat);

    std::cout << "[DFC::put] file_size : " << file_stat.st_size << std::endl;

    size_t file_size = file_stat.st_size;
    static char buf[1024];


    unsigned char hash[MD5_DIGEST_LENGTH];
    char* local_file_content = (char*)malloc(file_size);
    read(fd, local_file_content, file_size);
    lseek(fd, 0, SEEK_SET);
    MD5(reinterpret_cast<const unsigned char *>(local_file_content), file_size, hash);

    ssize_t wrt_size;
    char* header;

    int m11 = m_dfs4;
    int m12 = m_dfs1;

    int m21 = m_dfs1;
    int m22 = m_dfs2;

    int m31 = m_dfs2;
    int m32 = m_dfs3;

    int m41 = m_dfs3;
    int m42 = m_dfs4;

    unsigned char x = hash[MD5_DIGEST_LENGTH - 1] % 4;

    if (x == 1)
    {
        m11 = m_dfs1;
        m12 = m_dfs2;

        m21 = m_dfs2;
        m22 = m_dfs3;

        m31 = m_dfs3;
        m32 = m_dfs4;

        m41 = m_dfs4;
        m42 = m_dfs1;
    }
    else if (x == 2)
    {
        m11 = m_dfs2;
        m12 = m_dfs3;

        m21 = m_dfs3;
        m22 = m_dfs4;

        m31 = m_dfs4;
        m32 = m_dfs1;

        m41 = m_dfs1;
        m42 = m_dfs2;
    }
    else if (x == 3)
    {
        m11 = m_dfs3;
        m12 = m_dfs4;

        m21 = m_dfs4;
        m22 = m_dfs1;

        m31 = m_dfs1;
        m32 = m_dfs2;

        m41 = m_dfs2;
        m42 = m_dfs3;
    }
    // part 1
    // dfs4, dfs1
    std::string part_name = local_file + ".1";
    auto part_size = static_cast<ssize_t>(file_size / 4);
    std::tie(wrt_size, header) = put_header(part_name, part_size, subdir);
    ssize_t bytes_to_send = part_size;
    std::cout << "[DFC::put] part_size : " << part_size << std::endl;

    write(m11, header, wrt_size);
    write(m12, header, wrt_size);

    while (bytes_to_send > 0)
    {
        ssize_t n = read(fd, buf, bytes_to_send > sizeof(buf) ? sizeof(buf) : bytes_to_send);
        std::cout << "[DFC::put] bytes read: " << n << std::endl;
        write(m11, buf, n);
        write(m12, buf, n);
        bytes_to_send -= n;
    }
    free(header);

    // part 2
    // dfs1, dfs2
    part_name = local_file + ".2";
    std::tie(wrt_size, header) = put_header(part_name, part_size, subdir);
    bytes_to_send = part_size;

    write(m21, header, wrt_size);
    write(m22, header, wrt_size);

    while (bytes_to_send > 0)
    {
        ssize_t n = read(fd, buf, bytes_to_send > sizeof(buf) ? sizeof(buf) : bytes_to_send);
        write(m21, buf, n);
        write(m22, buf, n);
        bytes_to_send -= n;
    }
    free(header);

    // part 3
    // dfs2, dfs3
    part_name = local_file + ".3";
    std::tie(wrt_size, header) = put_header(part_name, part_size, subdir);
    bytes_to_send = part_size;

    write(m31, header, wrt_size);
    write(m32, header, wrt_size);

    while (bytes_to_send > 0)
    {
        ssize_t n = read(fd, buf, bytes_to_send > sizeof(buf) ? sizeof(buf) : bytes_to_send);
        write(m31, buf, n);
        write(m32, buf, n);
        bytes_to_send -= n;
    }

    // part 4
    // dfs3, dfs4
    part_name = local_file + ".4";
    part_size = static_cast<ssize_t>(file_size - part_size * 3);
    std::tie(wrt_size, header) = put_header(part_name, part_size, subdir);
    bytes_to_send = part_size;
    write(m41, header, wrt_size);
    write(m42, header, wrt_size);

    while (bytes_to_send > 0)
    {
        ssize_t n = read(fd, buf, bytes_to_send > sizeof(buf) ? sizeof(buf) : bytes_to_send);
        write(m41, buf, n);
        write(m42, buf, n);
        bytes_to_send -= n;
    }

    close(fd);
}