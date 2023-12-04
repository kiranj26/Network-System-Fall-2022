all: dfc

dfc: Conf.cpp Conf.h conn.cpp conn.h ListRequest.cpp ListRequest.h MkdirRequest.cpp MkdirRequest.h MultipleGetRequest.cpp MultipleGetRequest.h PutRequest.cpp PutRequest.h RetReader.cpp RetReader.cpp main.cpp
	g++ -std=c++17 -o dfc Conf.cpp conn.cpp ListRequest.cpp MkdirRequest.cpp MultipleGetRequest.cpp PutRequest.cpp RetReader.cpp main.cpp -lcrypto
