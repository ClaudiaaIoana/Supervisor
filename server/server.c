#define _XOPEN_SOURCE 700
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/signalfd.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define BUFFER_SIZE 1024
#define MAX_PROCESSES 256
#define MAX_CLIENTS 10

typedef struct Connection {
    __uint16_t port;
    char *ip;
} Connection;

typedef struct Client {
    int fd;
    char *ip;
    int port;
    char *host;
} Client;

typedef struct Manager {
    size_t process_count;
    size_t capacity;
} Manager;

typedef struct Task {
    int client_fd;
    void* (*function)(void*);
    void *args;
} Task;

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

int clients[MAX_CLIENTS];
pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;

time_t current_time;

int main(int argc, char *argv[]);
void init_manager_state(Manager *manager);
void add_process(Manager *manager, pid_t pid, const char *process_name);
void print_processes();
void print_clients();
void cleanup_state(Manager *manager);
void cleanup_connection(Connection *connection);
void log_entry(const char *message);
void *client_handler(void *args);
void *connection_handler(void* args);
void *tasks_handler(void* args);
void *server_command(void* args);
Client *get_client_info(int client_fd);

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    sigset_t mask; 
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);               
    sigaddset(&mask, SIGQUIT);               
    sigprocmask(SIG_BLOCK, &mask, NULL);
    int sfd = signalfd(-1, &mask, 0);

    Connection *connection = (Connection*) calloc (1, sizeof(Connection));
    DIE(connection == NULL, "calloc()");
    
    connection->ip = argv[1];
    connection->port = atoi(argv[2]);
    
    int rc = 0;
    pthread_t server_cmd;
    rc = pthread_create(&server_cmd, NULL, &server_command, &sfd);
    DIE(rc != 0, "pthread_create()");

    pthread_t connection_thread;
    rc = pthread_create(&connection_thread, NULL, &connection_handler, connection);
    DIE(rc != 0, "pthread_create()");
    
    // pthread_t tasks_thread;
    // rc = pthread_create(&tasks_thread, NULL, &tasks_handler, connection);
    // DIE(rc != 0, "pthread_create()");

    // pthread_t thread_pool[MAX_CLIENTS];
    // for (size_t i = 0; i < MAX_CLIENTS; i++) {
    //     rc = pthread_create(&thread_pool[i], NULL, &client_handler, manager);
    //     DIE(rc != 0, "pthread_create()");
    // }

    // pthread_join(connection_thread, NULL);
    // for(size_t i = 0; i < MAX_CLIENTS; i++) {
    //     pthread_join(thread_pool[i], NULL);
    // }

    pthread_join(connection_thread, NULL);
    pthread_join(server_cmd, NULL);
    
    //cleanup_state(manager);
    //free(manager);
    
    return EXIT_SUCCESS;
}

void *connection_handler(void *args)
{   
    Connection *connection = (Connection*) args;
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int listen_fd = 0;
    int num_clients = 0;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_fd < 0, "socket()");

    int enable = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    bzero(&serv_addr, socket_len);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(connection->port);
    int rc = inet_pton(AF_INET, connection->ip, &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");
    
    rc = bind(listen_fd, (const struct sockaddr*)&serv_addr, socket_len);
    DIE(rc < 0, "bind()");
    rc = listen(listen_fd, MAX_CLIENTS);
    DIE(rc < 0, "listen()");

    struct epoll_event ev;
    int epfd = epoll_create(MAX_CLIENTS);
    DIE(epfd < 0, "epoll_create()");
   
    ev.data.fd = listen_fd;        
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    
    int nfds = 0;
    int client_fd = 0;

    struct epoll_event events[MAX_CLIENTS];

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while(1) {
        nfds = epoll_wait(epfd, &events, MAX_CLIENTS, -1);
        DIE(nfds < 0, "epoll_wait");

        for(int i = 0; i < nfds; i++) {
            if ((events[i].data.fd == listen_fd) && ((events[i].events & EPOLLIN) != 0)) {
                client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                
                pthread_mutex_lock(&mutex_clients);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == 0) {
                        clients[i] = client_fd;
                        ev.data.fd = clients[i];        
                        ev.events = EPOLLIN|EPOLLRDHUP;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, clients[i], &ev);
                        num_clients++;
                        bzero(&client_addr, client_len);
                        Client *client_info = get_client_info(client_fd);
                        char message[BUFFER_SIZE];
                        sprintf(message, "INFO: connection on %s:%d %s", client_info->ip, client_info->port, client_info->host);
                        log_entry(message);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex_clients);
            }

            else {
                if (events[i].events & EPOLLIN) {
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] == events[i].data.fd) {
                            char message[BUFFER_SIZE];
                            Client *client_info = get_client_info(clients[i]);
                            sprintf(message, "INFO: received message on %s:%d %s", client_info->ip, client_info->port, client_info->host);
                            log_entry(message);
                            memset(message, 0, BUFFER_SIZE);
                            int rc = read(clients[i], message, BUFFER_SIZE);
                            DIE(rc < 0, "read()");
                            break;
                        }
                    }
                }

                if (events[i].events & EPOLLRDHUP) {
                    pthread_mutex_lock(&mutex_clients);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] == events[i].data.fd) {
                            char message[BUFFER_SIZE];
                            Client *client_info = get_client_info(clients[i]);
                            sprintf(message, "INFO: connection closed on %s:%d %s", client_info->ip, client_info->port, client_info->host);
                            log_entry(message);
                            close(clients[i]);
                            clients[i] = 0;
                            num_clients--;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&mutex_clients);
                } 
            }
        }       
    }
}

void *client_handler(void *args)
{

}

void *tasks_handler(void *args)
{
    
}

Client *get_client_info(int client_fd)
{
    Client *client_info = (Client*) calloc(1, sizeof(Client));
    DIE(client_info == NULL, "calloc()");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int rc = getpeername(client_fd, (struct sockaddr*)&client_addr, &client_len);
    DIE(rc < 0, "getpeername()");

    client_info->fd = client_fd;
    client_info->ip = inet_ntoa(client_addr.sin_addr);
    client_info->port = ntohs(client_addr.sin_port);
    struct hostent *host_info = gethostbyaddr(&(client_addr.sin_addr), sizeof(struct in_addr), AF_INET);
    client_info->host = host_info->h_name;

    return client_info;
}

void *server_command(void *args)
{
    char buffer[BUFFER_SIZE];
    int rc = 0;
    int sfd = *(int*)args;
    struct signalfd_siginfo fdsi;
    struct epoll_event ev;
    int epfd = epoll_create(2);

    ev.data.fd = STDIN_FILENO;        
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    
    ev.data.fd = sfd;        
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    struct epoll_event ret_ev;
    int nfds = 0;
    
    while(1) {

        printf("manager> ");
        fflush(stdout);

        nfds = epoll_wait(epfd, &ret_ev, 1, -1);
        DIE(nfds < 0, "epoll_wait");

        if ((ret_ev.data.fd == STDIN_FILENO) && ((ret_ev.events & EPOLLIN) != 0)) {
            memset(buffer, 0, 1024);
            int bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
            DIE(bytesRead < 0, "read()");
            buffer[bytesRead - 1] = 0;
            if(strcmp(buffer, "clients") == 0) {
                print_clients();
            }
            else if(strcmp(buffer, "processes") == 0) {
                print_processes();
            }
            else if(strcmp(buffer, "exit") == 0) {
                exit(0);
            }
            else {
                printf("Invalid command\n");
            }
        }

        else if ((ret_ev.data.fd == sfd) && ((ret_ev.events & EPOLLIN) != 0)) {
            int s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
            if (fdsi.ssi_signo == SIGINT) {
                exit(0);
            } else if (fdsi.ssi_signo == SIGQUIT) {
                exit(0);
            } else {
                printf("unexpected signal\n");
            }
        }
    }
}

void init_manager_state(Manager *manager)
{
    manager->process_count = 0;
    manager->capacity = 256;
}

void add_process(Manager *manager, pid_t pid, const char *process_name) {

}

void log_entry(const char *message) {

    pthread_mutex_lock(&mutex_log);
    FILE *log = fopen("manager.log", "a");
    time(&current_time);
    char *current_time_str = ctime(&current_time);
    current_time_str[strlen(current_time_str) - 1] = '\0';
    fprintf(log, "[%s] %s\n", current_time_str, message);
    fclose(log);
    pthread_mutex_unlock(&mutex_log);
}

void print_processes() {
    
}

void print_clients() {
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0) {
            Client *client_info = get_client_info(clients[i]);
            printf("%zu\t%s\t%d\t%s\n", i + 1, client_info->ip, client_info->port, client_info->host);
        }
    }
}

void cleanup_state(Manager *manager) {
    manager->process_count = 0;
    manager->capacity = 256;
}

void cleanup_connection(Connection *connection) {

}