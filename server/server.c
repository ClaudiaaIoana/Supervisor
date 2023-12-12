#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define BUFFER_SIZE 1024
#define MAX_PROCESSES 256
#define MAX_CLIENTS 10

typedef struct Process{
    pid_t pid;
    char *name;
    int client_fd;
} Process;

typedef struct Connection {
    __uint16_t port;
    struct sockaddr_in serv_addr;
    socklen_t socket_len;
    size_t num_clients;
    struct pollfd *listen;
    struct pollfd *clients;
} Connection;

typedef struct State{
    Connection* connection;
    Process *processes;
    size_t process_count;
    size_t capacity;
    FILE *log;
} State;

typedef struct Task {
    int client_fd;
    void* (*function)(void*);
    void *args;
} Task;

pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_clients = PTHREAD_COND_INITIALIZER;

time_t current_time;

int main(int argc, char *argv[]);
void init_connection(Connection *connection);
void init_manager_state(State *manager, Connection *connection);
void add_process(State *manager, pid_t pid, const char *process_name);
void print_processes(const State *manager);
void print_clients(const State *manager);
void cleanup_state(State *manager);
void cleanup_connection(Connection *connection);
void log_entry(State *manager, const char *message);
void *client_handler(void *args);
void *connection_handler(void* args);
void *server_command();

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    Connection *connection = (Connection*) calloc (1, sizeof(Connection));
    DIE(connection == NULL, "calloc()");
    connection->port = atoi(argv[2]);
    init_connection(connection);
    
    connection->listen->fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(connection->listen->fd < 0, "socket()");

    connection->socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if(setsockopt(connection->listen->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    memset(&connection->serv_addr, 0, connection->socket_len);
    connection->serv_addr.sin_family = AF_INET;
    connection->serv_addr.sin_port = htons(connection->port);

    int rc = inet_pton(AF_INET, argv[1], &connection->serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = bind(connection->listen->fd, (const struct sockaddr*)&connection->serv_addr, connection->socket_len);
    DIE(rc < 0, "bind()");

    pthread_t connection_thread;
    rc = pthread_create(&connection_thread, NULL, &connection_handler, connection);
    DIE(rc != 0, "pthread_create()");
    
    State *manager = (State*) calloc (1, sizeof(State));
    DIE(manager == NULL, "calloc()");

    init_manager_state(manager, connection);

    pthread_t thread_pool[MAX_CLIENTS];
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        rc = pthread_create(&thread_pool[i], NULL, &client_handler, manager);
        DIE(rc != 0, "pthread_create()");
    }

    pthread_t server_cmd;
    rc = pthread_create(&server_cmd, NULL, &server_command, manager);
    DIE(rc != 0, "pthread_create()");

    pthread_join(connection_thread, NULL);
    for(size_t i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(thread_pool[i], NULL);
    }
    pthread_join(server_cmd, NULL);
    
    cleanup_state(manager);
    free(manager);
    
    return EXIT_SUCCESS;
}

void *connection_handler(void *args)
{   
    Connection *connection = (Connection*) args;
    int rc = listen(connection->listen->fd, MAX_CLIENTS);
    DIE(rc < 0, "listen()");
    while(1) {
        rc = poll(connection->listen, 1, 0);
        DIE(rc < 0, "poll_listen()");
        if(connection->listen->revents & POLLIN) {
            printf("added client: %i\n", connection->listen->revents);
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int conn_fd = accept(connection->listen->fd, (struct sockaddr*)&client_addr, &client_len);
            DIE(conn_fd < 0, "accept()");
            printf("new conn_fd: %d\n", conn_fd);
            printf("num_clients: %d\n", connection->num_clients);
            //client->ip_address = inet_ntoa(client_addr.sin_addr));
            pthread_mutex_lock(&mutex_clients);
            for(int i = 0; i < MAX_CLIENTS; i++) {
                if(connection->clients[i].fd == 0) {
                    connection->clients[i].fd = conn_fd;
                    connection->clients[i].events = POLLIN | POLLRDHUP;
                    connection->num_clients++;
                    break;
                }
            }
            connection->listen->revents = 0;
            pthread_mutex_unlock(&mutex_clients); 
        }
        rc = poll(connection->clients, MAX_CLIENTS, 0);
        DIE(rc < 0, "poll_clients()");
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if(connection->clients[i].fd != 0) {
                if(connection->clients[i].revents & POLLRDHUP) {
                    printf("client disconnected\n");
                    pthread_mutex_lock(&mutex_clients);
                    close(connection->clients[i].fd);
                    connection->clients[i].fd = 0;
                    connection->num_clients--;
                    connection->clients[i].revents = 0;
                    pthread_mutex_unlock(&mutex_clients);
                }
            }
        }
    }
}

void *client_handler(void *args)
{
    State *manager = (State*) args;
    char buffer[BUFFER_SIZE];
    while(1) {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if(manager->connection->clients[i].fd != 0) {
                if(manager->connection->clients[i].revents & POLLIN) {
                    //printf("client %d sent data\n", manager->connection->clients[i].fd);
                    manager->connection->clients[i].revents = 0;
                    int rc = recv(manager->connection->clients[i].fd, buffer, BUFFER_SIZE, 0);
                    if(rc <= 0) {
                        break;
                    }
                    buffer[rc] = '\0';
                }
                // access information in JSON data
                //cJSON *json = cJSON_Parse(buffer);   
                //cJSON *pid = cJSON_GetObjectItem(json, "pid");
                //cJSON *process_name = cJSON_GetObjectItem(json, "name");
                //char message[32];
                //strcpy(message, "process launched");
                //log_entry(manager, message);
                //add_process(manager, pid->valueint, process_name->valuestring); 
            }
        }
    }
}

void *server_command(void *args)
{
    State *manager = (State*) args;
    char buffer[BUFFER_SIZE];

    while(1) {
        
        printf("supervisor> ");
        memset(buffer, 0 , BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        
        if(strcmp(buffer, "exit") == 0) {
            break;
        }
        else if(strcmp(buffer, "status") == 0) {
            print_processes(manager);
        }
        else if(strcmp(buffer, "clients") == 0) {
            print_clients(manager);
        } 
        else{
            printf("invalid command\n");
        }
    }
}

void init_connection(Connection *connection)
{
    connection->listen = calloc(1, sizeof(struct pollfd));
    DIE(connection->listen == NULL, "calloc()");
    connection->listen->events = POLLIN;
    connection->clients = calloc(MAX_CLIENTS, sizeof(struct pollfd));
    DIE(connection->clients == NULL, "calloc()");
    connection->num_clients = 0;
}

void init_manager_state(State *manager, Connection *connection)
{
    manager->connection = connection;
    manager->processes = calloc(manager->capacity, sizeof(Process));
    manager->process_count = 0;
    manager->capacity = 256;
    manager->log = fopen("manager.log", "w");
    DIE(manager->log == NULL, "fopen()");
    manager->connection->clients = calloc(MAX_CLIENTS, sizeof(struct pollfd));
    DIE(manager->connection->clients == NULL, "calloc()");
}

void add_process(State *manager, pid_t pid, const char *process_name) {
    Process *current_process = &manager->processes[manager->process_count++];
    current_process->name = calloc(32, sizeof(char));
    current_process->pid = pid;
    strcpy(current_process->name, process_name);
}

void log_entry(State *manager, const char *message) {
    time(&current_time);
    fprintf(manager->log, "%s > [%s]\n", ctime(&current_time), message);
    printf("logged success %s\n", message);
}

void print_processes(const State *manager) {
    printf("current processes for client:\n");
    for (size_t i = 0; i < manager->process_count; i++) {
        const Process *process = &manager->processes[i];
        printf("pid: <%d>, name: <%s>\n", process->pid, process->name);
    }
}

void print_clients(const State *manager) {
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if(manager->connection->clients[i].fd != 0) {
            printf("client: <%d>\n", manager->connection->clients[i].fd);
        }
    }
}

void cleanup_state(State *manager) {
    free(manager->processes);
    free(manager->connection->clients);
    manager->process_count = 0;
    manager->capacity = 256;
}

void cleanup_connection(Connection *connection) {
    free(connection->listen);
    free(connection->clients);
    connection->num_clients = 0;
}