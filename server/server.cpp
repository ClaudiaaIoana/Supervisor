#include "server.h"

server::server()
{
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0) {
        perror("socket");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(5454);

    this->thread_pool = new threadpool(3);
}

void server::init()
{
    running = true;

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    if(bind(server_fd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(1);
    }

    char hostname[24];
    gethostname(hostname, 24);

    if(listen(server_fd, 50) < 0) {
        perror("listen");
        exit(1);
    }

    std::cout << "server: running on " << hostname << " " << htons(server_address.sin_port) << "\n";
    std::cout << "man --list available commands\n";

    std::thread cli_thread(&server::handle_cli, this);
    cli_thread.detach();
}

server::~server() {
    delete thread_pool;
    close(server_fd);
}

void process_data(int client_fd) {
    char buffer[1024];
    int num_bytes = recv(client_fd, buffer, sizeof(buffer), 0);
    if (num_bytes > 0) {
        std::string received_message(buffer, num_bytes);
        std::cout << "client " << client_fd << ": " << received_message;
        rl_forced_update_display();
    } else {
        perror("recv");
    }
}


void server::handle_client(int client_fd) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_fd); //add client to list
    }

    pollfd poll_fd {client_fd, POLLIN | POLLRDHUP, 0}; //set pollfd for client

    while(true) {

        if(running == false) {
            break;
        }

        //loop until client disconnects 
        int num_ready = poll(&poll_fd, 1, -1);
        if(num_ready < 0) {
            perror("poll");
            break;
        }
        else if (num_ready == 0) {
            continue;
        }

        if(poll_fd.revents & POLLRDHUP) {
            std::cout << "client " << client_fd << " disconnected\n";
            //remove client from list 
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
            }

            close(client_fd);
            break;
        } 
        else
        {
           //process incoming data
           thread_pool->enqueue(process_data, client_fd); 
        }
    }
}

void server::accept_connections() {
    while(true) { //loop and accept incoming connections
        if(running == false) {
            break;
        }
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);
        int client_fd = accept(server_fd, (sockaddr*)&client_address, &client_address_size);
        if (client_fd < 0) {
            perror("accept failed\n");
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if(clients.size() >= MAX_CLIENTS) { //check if max number of clients has been reached
                std::cout << "max number of clients reached\n";
                close(client_fd);
                continue;
            }
        }
        thread_pool->enqueue(std::bind(&server::handle_client, this, client_fd)); //handle the new connection
    }
}

void server::handle_cli() {
    std::string input;
    while(true) {
        input = readline("> ");
        add_history(input.c_str());
        // Parse the command and client
        std::istringstream iss(input);
        std::string command;
        int client_fd;
        iss >> command >> client_fd;
        
        if(input == "clients") {
            std::cout << "Connected clients:" << std::endl; //list of connected clients
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (int client : clients) {
                    struct sockaddr_in addr;
                    socklen_t addr_len = sizeof(addr);
                    if (getpeername(client, (struct sockaddr*)&addr, &addr_len) == 0) {
                        std::cout << "client " << client << ": " << inet_ntoa(addr.sin_addr) << "|" << ntohs(addr.sin_port) << std::endl;
                    }
                }
            } 
        }

        else if(input == "man") {
            std::cout << "app <client> <app_name> --downloads and installs an app\n";
            std::cout << "auths <client> --get auth logs\n";
            std::cout << "clients <client> --lists connected clients\n";
            std::cout << "resources <client> --get resources usage\n"; 
            std::cout << "service <client> <service> --get running services\n"; 
            std::cout << "services <client> --get running services\n"; 
            std::cout << "ssh_logs <client> --get ssh logs\n"; 
            std::cout << "stats <client> --get stats\n"; 
            std::cout << "users <client> --get system users\n"; 
            std::cout << "exit --close the server\n";
        }

        else if(input == "exit") {
            {
                //close all client sockets
                std::lock_guard<std::mutex> lock(clients_mutex);
                for(int client : clients) {
                    close(client);
                }
            }
            running = false;
            delete thread_pool;
            rl_reset_terminal(NULL);
            exit(0);
        }

        else 
        {
            send_command(client_fd, command);
        }
    } 
}

void server::send_command(int client_fd, const std::string& command) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (std::find(clients.begin(), clients.end(), client_fd) != clients.end()) {
        // Send the command to the specified client
        std::string message = command + "\n";
        send(client_fd, message.c_str(), message.length(), 0);
    } else {
        std::cout << "invalid client: " << client_fd << std::endl;
    }
}