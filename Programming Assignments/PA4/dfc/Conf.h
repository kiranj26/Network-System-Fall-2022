//
// Created by young on 12/15/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_CONF_H
#define DISTRIBUTE_FILE_SYSTEM_CONF_H

#include <utility>
#include <string>
#include <iostream>
#include <fstream>


class Conf {
public:
    static Conf readConf(std::string& conf_file)
    {
        Conf conf;

        std::ifstream file(conf_file);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line))
            {
                if (line.substr(0, 6) == "Server")
                {
                    size_t start_pos = 7;
                    start_pos = line.find_first_not_of(' ', start_pos);
                    size_t end_pos = line.find_first_of(' ', start_pos);
                    std::string token = line.substr(start_pos, end_pos - start_pos);
//                std::cout << token << std::endl;
                    start_pos = end_pos + 1;
                    start_pos = line.find_first_not_of(' ', start_pos);
                    end_pos = line.find_first_of(':', start_pos);
                    std::string address = line.substr(start_pos, end_pos - start_pos);
//                std::cout << token << std::endl;
//                std::cout << line.substr(end_pos + 1) << std::endl;
                    int port = std::stoi(line.substr(end_pos + 1));

                    int idx = std::stoi(token.substr(token.length() - 1)) - 1;
                    conf.addresses[idx] = std::pair(address, port);
                }
                else if (line.substr(0, 8) == "Username")
                {
                    size_t pos = line.find_last_of(':');
                    std::string user = line.substr(pos + 2);
//                std::cout << user << std::endl;
                    conf.m_user = user;
                }
                else if (line.substr(0, 8) == "Password")
                {
                    size_t pos = line.find_last_of(':');
                    std::string password = line.substr(pos + 2);
//                std::cout << password << std::endl;
                    conf.m_password = password;
                }
            }
            file.close();
        }

        return conf;
    }

    std::pair<std::string, int> addresses[4];
    std::string m_user;
    std::string m_password;
};


#endif //DISTRIBUTE_FILE_SYSTEM_CONF_H
