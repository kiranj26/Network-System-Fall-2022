//
// Created by young on 12/16/21.
//

#include <fcntl.h>    // for open, O_RDONLY
#include <sys/stat.h> // for fstat
#include <unistd.h>   // read

#include <filesystem>

#include <openssl/md5.h>

#include "PutRequest.h"

namespace fs = std::filesystem;


void PutRequest::put() {
    std::cout << "[PutRequest::put] current path: " << fs::current_path() << std::endl;
    fs::path file_path = (fs::current_path() / fs::path(m_local_file));
    if (!fs::exists(file_path)) {
        std::cout << "file not found." << std::endl;
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    //                      load file into memory                            //
    //                    and calculate md5 of it                            //
    ///////////////////////////////////////////////////////////////////////////
    int fd = open(file_path.string().c_str(), O_RDONLY);

    struct stat file_stat{};
    fstat(fd, &file_stat);

    size_t file_size = file_stat.st_size;
    std::cout << "[PutRequest::put] file_size: " << file_size << std::endl;

    unsigned char hash[MD5_DIGEST_LENGTH];
    char *local_file_content = (char *) malloc(file_size);
    read(fd, local_file_content, file_size);
    lseek(fd, 0, SEEK_SET);
    MD5(reinterpret_cast<const unsigned char *>(local_file_content), file_size, hash);

    close(fd);

    ///////////////////////////////////////////////////////////////////////////
    //               encrypt the file content before sending                 //
    ///////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < file_size; i++) {
        local_file_content[i] ^= 77;
    }

    unsigned char x = hash[MD5_DIGEST_LENGTH - 1] % 4;
    std::cout << "[PutRequest::put] x (0 - 3): " << std::to_string(x) << std::endl;

    for (int server = 0; server < 4; server++) {
        int con_fd = conn(m_conf.addresses[server].first, m_conf.addresses[server].second);

        // sends at most 2 parts for each server
        for (auto part: {(server + 4 - x) % 4, (server - x + 5) % 4}) {
            auto part_size = static_cast<ssize_t>(file_size / 4);
            ssize_t start = part_size * part;
            ssize_t bytes_to_write = (part == 3) ? static_cast<ssize_t>(file_size - start) : part_size;

            ssize_t wrt_size;
            char *p_header;
            std::tie(wrt_size, p_header) = putHeader(part, bytes_to_write);

            ssize_t idx = start;
            if (con_fd > 0) {
                write(con_fd, p_header, wrt_size);
                while (bytes_to_write > 0) {
                    ssize_t n = write(con_fd,
                                      local_file_content + idx,
                                      bytes_to_write > 1024 ? 1024 : bytes_to_write);
                    bytes_to_write -= n;
                    idx += n;
                }
            }
            free(p_header);

            ///////////////////////////////////////////////////////////////////
            //                 read the return code                          //
            ///////////////////////////////////////////////////////////////////
            ssize_t ret;
            read(con_fd, &ret, sizeof(ret));
            if (ret == -2) {
                std::cout << "Invalid Username/Password. Please try again." << std::endl;
                break;
            }
        }
        close(con_fd);
    }
}


std::pair<ssize_t, char*> PutRequest::putHeader(int part, ssize_t part_size)
{
    std::string part_name = m_local_file + "." + std::to_string(part + 1);

    size_t header_size = 4;
    header_size += 8 + m_conf.m_user.length();
    header_size += 8 + m_conf.m_password.length();
    header_size += 8 + m_subdir.length();
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
    //                           user field                                  //
    ///////////////////////////////////////////////////////////////////////////
    size_t temp = m_conf.m_user.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_user.c_str(), m_conf.m_user.length());
    msg_pos += m_conf.m_user.length();

    ///////////////////////////////////////////////////////////////////////////
    //                          password field                               //
    ///////////////////////////////////////////////////////////////////////////
    temp = m_conf.m_password.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_conf.m_password.c_str(), m_conf.m_password.length());
    msg_pos += m_conf.m_password.length();

    ///////////////////////////////////////////////////////////////////////////
    // subdir
    temp = m_subdir.length();
    memcpy(msg_pos, &temp, sizeof(temp));
    msg_pos += sizeof(temp);

    memcpy(msg_pos, (void*)m_subdir.c_str(), m_subdir.length());
    msg_pos += m_subdir.length();

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