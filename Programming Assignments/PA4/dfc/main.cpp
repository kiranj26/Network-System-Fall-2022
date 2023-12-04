//
// Created by young on 12/15/21.
//
//#include "EventLoop.h"
#include <csignal>

#include <iostream>
//#include <fstream>
#include <fcntl.h>      // for open

#include <sys/socket.h> //socket
//#include <arpa/inet.h>  //inet_addr
//#include <netdb.h>      //hostent

#include <unistd.h>

#include <map>

#include "DFC.h"
#include "Conf.h"
#include "conn.h"
//#include "ListCollector.h"

#include "MultipleGetRequest.h"
#include "PutRequest.h"
#include "ListRequest.h"
#include "MkdirRequest.h"

// global event loop
//EventLoop* loop;


//void sig_int_handler(int signum) {
//    std::cout << "event loop is stopping" << std::endl;
//    loop->stop();
//}


int main(int argc, char** argv) {
//    signal(SIGINT, sig_int_handler);

    if (argc < 2) {
        std::cout << "usage: dfc dfc.conf" << std::endl;
        exit(0);
    }

    std::string conf_file = std::string(argv[1]);
    std::string line_input;

    ///////////////////////////////////////////////////////////////////////////
    //                          parsing conf file                            //
    ///////////////////////////////////////////////////////////////////////////
    Conf conf = Conf::readConf(conf_file);

    for(auto& address: conf.addresses)
    {
        std::cout << address.first << ", " << address.second << std::endl;
    }
    std::cout << "Username: " << conf.m_user << std::endl;
    std::cout << "Password: " << conf.m_password << std::endl;

    std::string cur_dir = ".";

    ///////////////////////////////////////////////////////////////////////////
    //                           take inputs                                 //
    ///////////////////////////////////////////////////////////////////////////
    while (true)
    {
        std::cout << "# ";
        std::getline(std::cin, line_input);

        if (line_input.length() == 0)
            continue;

        if (line_input.length() > 3 && line_input.substr(0,4) == "list")
        {
            ///////////////////////////////////////////////////////////////////
            //                       list command!                           //
            ///////////////////////////////////////////////////////////////////
            size_t start_pos = 4, end_pos;
            std::string subdir = ".";
            int num_args = 0;

            while (true)
            {
                start_pos = line_input.find_first_not_of(' ', start_pos);
                if (start_pos == std::string::npos)
                    break;

//                std::cout << "start_pos: " << start_pos << std::endl;
                end_pos = line_input.find_first_of(' ', start_pos);
//                std::cout << "end_pos: " << end_pos << std::endl;

                if (end_pos == std::string::npos)
                {
//                    std::cout << "last token: " << line_input.substr(start_pos) << std::endl;
                    num_args += 1;
                    if (num_args == 1)
                        subdir = line_input.substr(start_pos);
                    if (num_args > 1)
                        std::cout << "too many arguments" << std::endl;
                    break;
                }
//                std::cout << line_input.substr(start_pos, end_pos - start_pos) << std::endl;
                if (num_args == 0)
                    subdir = line_input.substr(start_pos, end_pos - start_pos);
                if (num_args > 0) {
                    std::cout << "too many arguments" << std::endl;
                    break;
                }
                num_args += 1;

                start_pos = end_pos;
            }

            ListRequest lr(conf, subdir);
            lr.list();
        }
        else if (line_input.length() > 2 && line_input.substr(0,3) == "get")
        {
            ///////////////////////////////////////////////////////////////////
            //                      get command!                             //
            ///////////////////////////////////////////////////////////////////
            if (line_input[3] != ' ')
            {
                std::cout << "Command not found" << std::endl;
                continue;
            }

            size_t start_pos = 3, end_pos;
            std::string file_name;
            std::string subdir = ".";
            int num_args = 0;

            while (true)
            {
                start_pos = line_input.find_first_not_of(' ', start_pos);
                if (start_pos == std::string::npos)
                    break;

//                std::cout << "start_pos: " << start_pos << std::endl;
                end_pos = line_input.find_first_of(' ', start_pos);
//                std::cout << "end_pos: " << end_pos << std::endl;

                if (end_pos == std::string::npos)
                {
//                    std::cout << "last token: " << line_input.substr(start_pos) << std::endl;
                    num_args += 1;
                    if (num_args == 1)
                        file_name = line_input.substr(start_pos);
                    if (num_args == 2)
                        subdir = line_input.substr(start_pos);
                    if (num_args > 2)
                        std::cout << "too many arguments" << std::endl;
                    break;
                }
//                std::cout << line_input.substr(start_pos, end_pos - start_pos) << std::endl;
                if (num_args == 0)
                    file_name = line_input.substr(start_pos, end_pos - start_pos);
                if (num_args == 1)
                    subdir = line_input.substr(start_pos, end_pos - start_pos);
                if (num_args > 1) {
                    std::cout << "too many arguments" << std::endl;
                    break;
                }
                num_args += 1;

                start_pos = end_pos;
            }

            if (num_args < 3)
            {
//                std::cout << "GET file_name(" << file_name << "), subdir(" << subdir << ")" << std::endl;
                ///////////////////////////////////////////////////////////////
                // actual GET works here!
                ///////////////////////////////////////////////////////////////
                std::string content;

                std::string part_content;
                bool available;

                MultipleGetRequest request1(conf);
                std::tie(available, part_content) = request1.requestFile(file_name + ".1", subdir);
                if (!available)
                {
                    std::cout << "file is not available" << std::endl;
                    continue;
                }
                content += part_content;

                MultipleGetRequest request2(conf);
                std::tie(available, part_content) = request2.requestFile(file_name + ".2", subdir);
                if (!available)
                {
                    std::cout << "file is not available" << std::endl;
                    continue;
                }
                content += part_content;

                MultipleGetRequest request3(conf);
                std::tie(available, part_content) = request3.requestFile(file_name + ".3", subdir);
                if (!available)
                {
                    std::cout << "File is incomplete" << std::endl;
                    continue;
                }
                content += part_content;

                MultipleGetRequest request4(conf);
                std::tie(available, part_content) = request4.requestFile(file_name + ".4", subdir);
                if (!available)
                {
                    std::cout << "file is not available" << std::endl;
                    continue;
                }
                content += part_content;

                int fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
                size_t bytes_to_write = content.length();
                size_t written_bytes = 0;

                while (bytes_to_write > 0)
                {
                    ssize_t n = write(fd, content.c_str() + written_bytes, bytes_to_write > 1024 ? 1024 : bytes_to_write);
                    written_bytes += n;
                    bytes_to_write -= n;
                }

                close(fd);

//                std::cout << content.length() << std::endl;
            }
        }
        else if (line_input.length() > 2 && line_input.substr(0,3) == "put")
        {
            ///////////////////////////////////////////////////////////////////
            //                        put command!                           //
            ///////////////////////////////////////////////////////////////////
            size_t pos_start = 4, pos_end;

            int num_args = 0;
            std::string local_file;
            std::string sub_dir = ".";

            while (true)
            {
                pos_end = line_input.find(' ', pos_start);
                std::string token = line_input.substr(pos_start, pos_end - pos_start);
                if (!token.empty()) {
                    if (num_args == 0)
                    {
                        local_file = token;
                        num_args += 1;
                    }
                    else if (num_args == 1)
                    {
                        sub_dir = token;
                        num_args += 1;
                    }
                    else
                    {
                        break;
                    }
                }
                pos_start = pos_end + 1;

                if (pos_end == std::string::npos)
                    break;
            }

            if (num_args < 1)
            {
                std::cout << "not file is given" << std::endl;
            }
            else if (num_args > 2)
            {
                std::cout << "too many arguments" << std::endl;
            }
            else
            {
//                dfc.put(local_file, sub_dir);
                ///////////////////////////////////////////////////////////////
                //
                //  actually PUT works happens here!
                //
                ///////////////////////////////////////////////////////////////
                PutRequest putRequest(conf, local_file, sub_dir);
                putRequest.put();

            }
        }
        else if (line_input.length() > 4 && line_input.substr(0,5) == "mkdir")
        {
            ///////////////////////////////////////////////////////////////////
            //                       mkdir command!                          //
            ///////////////////////////////////////////////////////////////////
            size_t start_pos = 5, end_pos;
            start_pos = line_input.find_first_not_of(' ', start_pos);
//            std::cout << "start_pos: " << start_pos << std::endl;

            if (start_pos == std::string::npos)
            {
                std::cout << "missing directory" << std::endl;
                continue;
            }

            end_pos = line_input.find_first_of(' ', start_pos);
//            std::cout << "end_pos: " << end_pos << std::endl;
            if (end_pos == std::string::npos)
            {
                ///////////////////////////////////////////////////////////////
                std::cout << "[main] mkdir: " << line_input.substr(start_pos, end_pos - start_pos) << std::endl;
                std::string subdir = line_input.substr(start_pos, end_pos - start_pos);
                MkdirRequest request(conf, subdir);
                request.mkdir();
            }
            else
            {
                std::cout << "too many arguments" << std::endl;
            }

        }
        else if (line_input == "exit")
        {
            break;
        }
        else
        {
            std::cout << "command not found" << std::endl;
        }
    }

    return 0;
}