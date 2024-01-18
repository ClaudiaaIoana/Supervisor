#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024
#define MAX_EVENTS 10
#define MAX_CLIENTS 10
#define MAX_TASK_QUEUE 10

typedef struct Process {
    pid_t pid;
    char *name;
    char *status;
    int client_fd;
} Process;

typedef struct Connection {
    __uint16_t port;
    struct sockaddr_in serv_addr;
    socklen_t socket_len;
    size_t num_clients;
    int epoll_fd;
    struct epoll_event *events;
    struct epoll_event listen_event;
    int listen_fd;
    pthread_t client_thread;
} Connection;

typedef struct State {
    Connection *connection;
    Process *processes;
    size_t process_count;
    size_t capacity;
    FILE *log;
} State;

typedef struct Task {
    int client_fd;
    void (*function)(State *, int);
} Task;

typedef struct ThreadPool {
    pthread_t threads[MAX_CLIENTS];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Task task_queue[MAX_TASK_QUEUE];
    size_t task_count;
} ThreadPool;

ThreadPool thread_pool;

pthread_t stdin_thread;

void *client_thread(void *args);
void *stdin_thread_func(void *args);
void *worker_thread(void *args);
void *connection_handler(void *args);

void init_connection(Connection *connection);
void init_manager_state(State *manager, Connection *connection);
void cleanup_state(State *manager);
void cleanup_connection(Connection *connection);
void init_thread_pool();
void enqueue_task(Task *task);
Task dequeue_task();

void add_process(State *manager, pid_t pid, const char *process_name, const char *status);

void log_entry(State *manager, const char *message);

void process_client_data(State *manager, int client_fd);
void add_client(Connection *connection, int client_fd);
void remove_client(Connection *connection, int client_fd);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    Connection *connection = (Connection *)calloc(1, sizeof(Connection));
    if (connection == NULL) {
        perror("calloc()");
        exit(EXIT_FAILURE);
    }

    connection->port = atoi(argv[2]);
    init_connection(connection);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, connection_handler, connection);

    pthread_t stdin_thread;
    pthread_create(&stdin_thread, NULL, stdin_thread_func, NULL);

    init_thread_pool();

    State *manager = (State *)calloc(1, sizeof(State));
    if (manager == NULL) {
        perror("calloc()");
        exit(EXIT_FAILURE);
    }

    init_manager_state(manager, connection);

    pthread_t thread_pool[MAX_CLIENTS];
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        pthread_create(&thread_pool[i], NULL, &worker_thread, manager);
    }

    pthread_join(client_thread, NULL);
    pthread_join(stdin_thread, NULL);

    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    cleanup_state(manager);
    free(manager);

    cleanup_connection(connection);
    free(connection);

    return EXIT_SUCCESS;
}

void *connection_handler(void *args) {
    Connection *connection = (Connection *)args;
    struct epoll_event event;
    while (1) {
        int num_events = epoll_wait(connection->epoll_fd, connection->events, MAX_EVENTS, -1);
        if (num_events < 0) {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < num_events; i++) {
            if (connection->events[i].data.fd == connection->listen_fd) {
                int client_fd = accept(connection->listen_fd, NULL, NULL);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(connection->epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }

                add_client(connection, client_fd);
            } else if (connection->events[i].events & EPOLLIN) {
                int client_fd = connection->events[i].data.fd;
                Task task;
                task.client_fd = client_fd;
                task.function = process_client_data;
                enqueue_task(&task);
            }
        }
    }

    return NULL;
}

void *stdin_thread_func(void *args) {
    char buffer[BUFFER_SIZE];
    while (1) {
        printf("server> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';

        // Process stdin commands
        if (strcmp(buffer, "exit") == 0) {
            break;
        } else {
            printf("Invalid command\n");
        }
    }

    return NULL;
}

void *worker_thread(void *args) {
    State *manager = (State *)args;
    while (1) {
        Task task = dequeue_task();
        task.function(manager, task.client_fd);
    }
}

void init_connection(Connection *connection) {
    connection->epoll_fd = epoll_create1(0);
    if (connection->epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    connection->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection->listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(connection->listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EXIT_FAILURE);
    }

    memset(&connection->serv_addr, 0, sizeof(connection->serv_addr));
    connection->serv_addr.sin_family = AF_INET;
    connection->serv_addr.sin_port = htons(connection->port);

    int rc = inet_pton(AF_INET, "127.0.0.1", &connection->serv_addr.sin_addr.s_addr);
    if (rc <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    rc = bind(connection->listen_fd, (const struct sockaddr *)&connection->serv_addr, sizeof(connection->serv_addr));
    if (rc == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    rc = listen(connection->listen_fd, MAX_CLIENTS);
    if (rc == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    connection->listen_event.events = EPOLLIN | EPOLLET;
    connection->listen_event.data.fd = connection->listen_fd;
    if (epoll_ctl(connection->epoll_fd, EPOLL_CTL_ADD, connection->listen_fd, &connection->listen_event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    connection->events = calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (connection->events == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
}

void init_manager_state(State *manager, Connection *connection) {
    manager->connection = connection;
    manager->processes = calloc(manager->capacity, sizeof(Process));
    if (manager->processes == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    manager->process_count = 0;
    manager->capacity = 256;
    manager->log = fopen("manager.log", "a");
    if (manager->log == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
}

void cleanup_state(State *manager) {
    free(manager->processes);
    fclose(manager->log);
}

void cleanup_connection(Connection *connection) {
    close(connection->listen_fd);
    close(connection->epoll_fd);
    free(connection->events);
}

void init_thread_pool() {
    thread_pool.task_count = 0;
    pthread_mutex_init(&thread_pool.mutex, NULL);
    pthread_cond_init(&thread_pool.cond, NULL);

    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        pthread_create(&thread_pool.threads[i], NULL, worker_thread, NULL);
    }
}

void enqueue_task(Task *task) {
    pthread_mutex_lock(&thread_pool.mutex);

    while (thread_pool.task_count >= MAX_TASK_QUEUE) {
        pthread_cond_wait(&thread_pool.cond, &thread_pool.mutex);
    }

    thread_pool.task_queue[thread_pool.task_count++] = *task;
    pthread_cond_signal(&thread_pool.cond);
    pthread_mutex_unlock(&thread_pool.mutex);
}

Task dequeue_task() {
    pthread_mutex_lock(&thread_pool.mutex);

    while (thread_pool.task_count == 0) {
        pthread_cond_wait(&thread_pool.cond, &thread_pool.mutex);
    }

    Task task = thread_pool.task_queue[--thread_pool.task_count];

    pthread_cond_signal(&thread_pool.cond);
    pthread_mutex_unlock(&thread_pool.mutex);

    return task;
}

void add_process(State *manager, pid_t pid, const char *process_name, const char *status) {
    pthread_mutex_lock(&thread_pool.mutex);
    Process *current_process = &manager->processes[manager->process_count++];
    current_process->name = strdup(process_name);
    current_process->status = strdup(status);
    current_process->pid = pid;
    pthread_mutex_unlock(&thread_pool.mutex);

    log_entry(manager, "Process added");
}

void log_entry(State *manager, const char *message) {
    pthread_mutex_lock(&thread_pool.mutex);
    time_t current_time;
    FILE *log = fopen("manager.log", "a");
    if (log == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&thread_pool.mutex);
        return;
    }
    time(&current_time);
    fprintf(log, "%s > [%s]\n", ctime(&current_time), message);
    fclose(log);
    pthread_mutex_unlock(&thread_pool.mutex);
}

void process_client_data(State *manager, int client_fd) {
    char buffer[BUFFER_SIZE];
    int rc = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (rc <= 0) {
        remove_client(manager->connection, client_fd);
        close(client_fd);
        return;
    }

    buffer[rc] = '\0';
    // cJSON *json = cJSON_Parse(buffer);
    // cJSON *pid = cJSON_GetObjectItem(json, "pid");
    // cJSON *process_name = cJSON_GetObjectItem(json, "name");
    // cJSON *status = cJSON_GetObjectItem(json, "status");

    //add_process(manager, pid->valueint, process_name->valuestring, status->valuestring);
    //cJSON_Delete(json);
}

void add_client(Connection *connection, int client_fd) {
    pthread_mutex_lock(&thread_pool.mutex);
    if (connection->num_clients < MAX_CLIENTS) {
        connection->events[connection->num_clients].events = EPOLLIN | EPOLLET;
        connection->events[connection->num_clients].data.fd = client_fd;
        if (epoll_ctl(connection->epoll_fd, EPOLL_CTL_ADD, client_fd, &connection->events[connection->num_clients]) == -1) {
            perror("epoll_ctl");
            close(client_fd);
            pthread_mutex_unlock(&thread_pool.mutex);
            return;
        }
        connection->num_clients++;
        //log_entry(manager, "Client connected");
    }
    pthread_mutex_unlock(&thread_pool.mutex);
}

void remove_client(Connection *connection, int client_fd) {
    pthread_mutex_lock(&thread_pool.mutex);
    for (size_t i = 0; i < connection->num_clients; i++) {
        if (connection->events[i].data.fd == client_fd) {
            if (epoll_ctl(connection->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                perror("epoll_ctl");
            }
            //log_entry(manager, "Client disconnected");
            for (size_t j = i; j < connection->num_clients - 1; j++) {
                connection->events[j] = connection->events[j + 1];
            }
            connection->num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&thread_pool.mutex);
}