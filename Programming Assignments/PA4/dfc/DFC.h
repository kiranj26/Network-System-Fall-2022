//
// Created by young on 12/15/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_DFC_H
#define DISTRIBUTE_FILE_SYSTEM_DFC_H

#include <string>
#include <utility>


class DFC {
public:
    DFC(std::string user, std::string password, int dfs1, int dfs2, int dfs3, int dfs4) :
    m_user(std::move(user)), m_password(std::move(password)),
    m_dfs1(dfs1), m_dfs2(dfs2), m_dfs3(dfs3), m_dfs4(dfs4) {}

    void list(std::string& subdir);
    void put(std::string& local_file, std::string& subdir);
private:
    std::pair<ssize_t, char*> put_header(std::string& part_name, ssize_t part_size, std::string& subdir);
    std::string m_user;
    std::string m_password;
    int m_dfs1;
    int m_dfs2;
    int m_dfs3;
    int m_dfs4;
};


#endif //DISTRIBUTE_FILE_SYSTEM_DFC_H
