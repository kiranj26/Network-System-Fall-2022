//
// Created by young on 12/15/21.
//
#include "conn.h"
#include <iostream>


int conn(std::string address, int port)
{
    struct sockaddr_in server{};

    int sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        perror("Could not create socket");
    }

//    std::cout<<"Socket created\n";

    // setup address structure
    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *he;
        struct in_addr **addr_list;

        //resolve the hostname, its not an ip address
        if ( (he = gethostbyname( address.c_str() ) ) == nullptr)
        {
            //gethostbyname failed
            herror("gethostbyname");
            std::cout << "Failed to resolve hostname\n";

            return -1;
        }

        // Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
        addr_list = (struct in_addr **) he->h_addr_list;

        for(int i = 0; addr_list[i] != nullptr; i++)
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];

            std::cout << address << " resolved to " << inet_ntoa(*addr_list[i]) << std::endl;

            break;
        }
    }
    else
    {
        //plain ip address
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }

    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    //Connect to remote server
    if( connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0 )
    {
//        perror("connect failed. Error");
        return -1;
    }

//    std::cout << "Connected\n";
    return sock;
}