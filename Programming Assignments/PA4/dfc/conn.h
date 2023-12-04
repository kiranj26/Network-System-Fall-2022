//
// Created by young on 12/15/21.
//

#ifndef DISTRIBUTE_FILE_SYSTEM_CONN_H
#define DISTRIBUTE_FILE_SYSTEM_CONN_H

#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <netdb.h> //hostent
#include <string>

int conn(std::string address, int port);

#endif //DISTRIBUTE_FILE_SYSTEM_CONN_H