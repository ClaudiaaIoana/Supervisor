#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define BUFFER_SIZE 1024
#define MAX_PROCESSES 256
#define MAX_CLIENTS 2

typedef struct Process{
    pid_t pid;
    char *name;
} Process;

typedef struct State{
    Process *processes;
    size_t process_count;
    size_t capacity;
} State;

void init_manager_state(State *manager);
void add_process(State *manager, pid_t pid, const char *process_name);
void print_processes(const State *manager);
void cleanup_state(State *manager);

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    __uint16_t port = 4444;
    int rc = sscanf(argv[2], "%hu", &port);
    DIE(rc != 1, "invalid port");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_fd < 0, "socket()");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = bind(listen_fd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind()");

    rc = listen(listen_fd, 0);
    DIE(rc < 0, "listen()");

    printf("Waiting for client...\n");
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    DIE(conn_fd < 0, "accept()");

    printf("Client connected...\n");

    State *manager = (State*) calloc (1, sizeof(State));
    init_manager_state(manager);

    char buffer[1024];
    while(1) {
        printf("Waiting...\n");
        int rc = recv(conn_fd, buffer, 1024, 0);
        if(rc <= 0) {
            break;
        }
        // access pid in the JSON data
        cJSON *json = cJSON_Parse(buffer);   
        cJSON *pid = cJSON_GetObjectItem(json, "pid");
        cJSON *process_name = cJSON_GetObjectItem(json, "name");
        add_process(manager, pid->valueint, process_name->valuestring); 
    }

    print_processes(manager);

    cleanup_state(manager);
    free(manager);
    rc = close(conn_fd);
    DIE(rc < 0, "close()");

    return EXIT_SUCCESS;
}

void init_manager_state(State *manager) {
    manager->processes = calloc(manager->capacity, sizeof(Process));
    manager->process_count = 0;
    manager->capacity = 256;
}

void add_process(State *manager, pid_t pid, const char *process_name) {
    Process *current_process = &manager->processes[manager->process_count++];
    current_process->name = calloc(32, sizeof(char));
    current_process->pid = pid;
    strcpy(current_process->name, process_name);
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