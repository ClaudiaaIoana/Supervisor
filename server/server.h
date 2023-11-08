#pragma once

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>  
//#include <readline/readline.h>
//#include <readline/history.h>
#include <vector>
#include <poll.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <map>
#include "command.h"
#include "threadpool.h"

#define MAX_CLIENTS 10

class server {
    threadpool *thread_pool;
    std::atomic<bool> running;
    int server_fd;
    std::vector<int> clients;
    std::mutex clients_mutex;
    struct sockaddr_in server_address { 0 };

public:
    server();
    ~server();
    void init();
    void accept_connections();
    void handle_client(int client_fd);
    void handle_cli();
};