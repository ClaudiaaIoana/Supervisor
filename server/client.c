#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define BUFFER_SIZE 1024

struct chat_packet {
  uint16_t len;
  char message[BUFFER_SIZE + 1];
};

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    __uint16_t port = 4444;
    int rc = sscanf(argv[2], "%hu", &port);
    DIE(rc != 1, "invalid port");

    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(conn_fd < 0, "socket()");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if(setsockopt(conn_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    rc = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect()");

    printf("Connected to the server...\n");

    //launch the process

    char buffer[1024];

    cJSON *json = cJSON_CreateObject(); 
    cJSON_AddStringToObject(json, "name", "John Doe"); 
    cJSON_AddNumberToObject(json, "pid", 3003); 
    cJSON_AddStringToObject(json, "email", "john.doe@example.com"); 

    char *json_str = cJSON_Print(json); 

    while(1) {
        printf("Type your command: ");
        memset(buffer, 0, 1024);
        fgets(buffer, 1024, stdin);
        DIE(rc < 0, "scanf()");
        if(strncmp(buffer, "send", 4) == 0) {
            rc = send(conn_fd, json_str, 1024, 0);
            DIE(rc < 0, "send()");
        }
        
        if(strncmp(buffer, "exit", 4) == 0) {
            break;
        }
    }
    
    rc = close(conn_fd);
    DIE(rc < 0, "close()");
    return EXIT_SUCCESS;
}