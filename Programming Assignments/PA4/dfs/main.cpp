//
// Created by young on 12/14/21.
//

#include "DFS.h"
#include "EventLoop.h"
#include <csignal>

#include <iostream>
#include <fstream>

#include <map>


// global event loop
EventLoop* loop;


void sig_int_handler(int signum) {
    std::cout << "event loop is stopping" << std::endl;
    loop->stop();
}


int main(int argc, char** argv) {
    signal(SIGINT, sig_int_handler);

    // when a socket is closed by a client, you may get SIGPIPE signals
    // is it the default behavior is to terminate the application? <- not good!
    // so ignore them. I can handle this error myself
    signal(SIGPIPE, SIG_IGN);

    if (argc < 3) {
        std::cout << "usage: dfs directory port" << std::endl;
        exit(0);
    }

    std::string directory = std::string(argv[1]);
    int port = std::stoi(std::string(argv[2]));

    std::map<std::string, std::string> users;

    std::ifstream file("./dfs.conf");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // using printf() in all tests for consistency
//            std::cout << line << ", " << line.find_first_of(' ') << ", " << line.find_last_of(' ') << std::endl;
            size_t a, b;
            a = line.find_first_of(' ');
            b = line.find_last_of(' ');
            std::string user = line.substr(0, a);
            std::string password = line.substr(b + 1, -1);
            std::cout << user << "," << password << std::endl;
            users[user] = password;
        }
        file.close();
    }

    auto dfs = new DFS(directory, port, users);

    loop = new EventLoop();
    loop->addHandler(dfs);
    loop->start();

    std::cout << "event loop stopped" << std::endl;

    // when the event loop is destroyed, all registered handlers
    // are also destroyed along with it
    delete loop;

    return 0;
}