#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define BUFFER_SIZE 1024
#define MAX_PROCESSES 256
#define MAX_CLIENTS 10

typedef struct Process{
    pid_t pid;
    char *name;
} Process;

typedef struct State{
    Process *processes;
    size_t process_count;
    size_t capacity;
    FILE *log;
} State;

typedef struct Connection {
    __uint16_t port;
    struct sockaddr_in serv_addr;
    socklen_t socket_len;
    int listen_fd;
    
} Connection;

pthread_mutex_t mutex_clients = PTHREAD_MUTEX_INITIALIZER;

int clients_fd[MAX_CLIENTS] = { 0 };

time_t current_time;

int main(int argc, char * argv[]);
void init_manager_state(State *manager);
void add_process(State *manager, pid_t pid, const char *process_name);
void print_processes(const State *manager);
void cleanup_state(State *manager);
void log_entry(State *manager, const char *message);
void *client_handler(void *args);
void *connection_handler(void* args);
void *server_command();

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    Connection *connection = (Connection*) calloc(1, sizeof(Connection));
    DIE(connection == NULL, "calloc()");
    connection->port = atoi(argv[2]);

    connection->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(connection->listen_fd < 0, "socket()");

    connection->socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if(setsockopt(connection->listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    memset(&connection->serv_addr, 0, connection->socket_len);
    connection->serv_addr.sin_family = AF_INET;
    connection->serv_addr.sin_port = htons(connection->port);

    int rc = inet_pton(AF_INET, argv[1], &connection->serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = bind(connection->listen_fd, (const struct sockaddr*)&connection->serv_addr, connection->socket_len);
    DIE(rc < 0, "bind()");

    pthread_t connection_thread;
    rc = pthread_create(&connection_thread, NULL, &connection_handler, connection);
    DIE(rc != 0, "pthread_create()");
    
    State *manager = (State*) calloc (1, sizeof(State));
    DIE(manager == NULL, "calloc()");

    init_manager_state(manager);

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
    int num_clients = 0;
    Connection *connection = (Connection*) args;
    int rc = listen(connection->listen_fd, MAX_CLIENTS);
    DIE(rc < 0, "listen()");

    printf("Waiting for client...\n");
    while(1) {
        if(num_clients == MAX_CLIENTS) {
            break;
        }
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int conn_fd = accept(connection->listen_fd, (struct sockaddr*)&client_addr, &client_len);
        DIE(conn_fd < 0, "accept()");

        pthread_mutex_lock(&mutex_clients);
        clients_fd[num_clients] = conn_fd;
        num_clients++;
        pthread_mutex_unlock(&mutex_clients);
        printf("New client connected...\n");
    }
}

void *client_handler(void *args)
{
    State *manager = (State*) args;
    char buffer[BUFFER_SIZE];
    while(1) {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if(clients_fd[i] != 0) {
                int rc = recv(clients_fd[i], buffer, BUFFER_SIZE, 0);
                if(rc <= 0) {
                    break;
                }
                buffer[rc] = '\0';
                // access information in JSON data
                cJSON *json = cJSON_Parse(buffer);   
                cJSON *pid = cJSON_GetObjectItem(json, "pid");
                cJSON *process_name = cJSON_GetObjectItem(json, "name");
                char message[32];
                strcpy(message, "process launched");
                log_entry(manager, message);
                add_process(manager, pid->valueint, process_name->valuestring); 
            }
        }
    }
}

void *server_command(void *args)
{
    State *manager = (State*) args;
    char buffer[BUFFER_SIZE];
    while(1) {
        printf("> ");
        fflush(stdout);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';
        if(strcmp(buffer, "exit") == 0) {
            break;
        }
        else if(strcmp(buffer, "status") == 0) {
            print_processes(manager);
        }
        else {
            printf("invalid command\n");
        }
    }
}

void init_manager_state(State *manager)
{
    manager->processes = calloc(manager->capacity, sizeof(Process));
    manager->process_count = 0;
    manager->capacity = 256;
    manager->log = fopen("manager.log", "a");
    DIE(manager->log == NULL, "fopen()");
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
    printf("current processes for connected client:\n");
    for (size_t i = 0; i < manager->process_count; i++) {
        const Process *process = &manager->processes[i];
        printf("pid: <%d>, name: <%s>\n", process->pid, process->name);
    }
}

void cleanup_state(State *manager) {
    free(manager->processes);
    manager->process_count = 0;
    manager->capacity = 256;
}