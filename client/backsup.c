#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define SOCK_PATH "/tmp/mysocket"

// when working with pipes
#define READ_END 0
#define WRITE_END 1

// when working with standard fd
#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

//key words
#define W_DIR       0
#define W_STDIN     1
#define W_STDOUT    2
#define W_STDERR    3
#define W_PRIORITY  4
#define W_UMASK     5
#define W_GOWNER    6
#define W_OWNER     7

struct Setting{
    int     set_w;
    char    parameters[200];

}typedef Setting;

struct Connection{
    char ip[16];
    char port[6];
}typedef Connection_t;

void create_child_proccess(const char *filename);

void launch_group(const char *filename);

void redirect(char *parameters, int fd_redirected);

void change_owner(uid_t uid);

void change_gowner(gid_t gid);

void change_priority(int new_priority);

void change_umask(mode_t new_umask);

void change_exec_directory(char *path_to_directory);

int key_word(char* word);

int get_lines(FILE* file);

int compare_settings(const void *a, const void *b);

void _erase_front_spaces(char** string);

char* analise_config(const char *filename);

void* communicate_fsup(void *arg);

void* handle_client(void *arg);

void* communicate_manager(void *arg);


int main(int argc, char* argv[])
{
    pthread_t thread_id[2];
    if (pthread_create(&thread_id[0], NULL, communicate_fsup, (void *)NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    printf("Created thread 1\n");

    Connection_t *conn_param=(Connection_t*)malloc(sizeof(Connection_t));
    strcpy(conn_param->ip,argv[1]);
    strcpy(conn_param->port,argv[2]);

    if (pthread_create(&thread_id[1], NULL, communicate_manager, (void *)conn_param) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    printf("Created thread 2\n");

    if (pthread_join(thread_id[0], NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    printf("Exited thread!\n");

    if (pthread_join(thread_id[1], NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    //create_child_proccess("test1.conf");

    int status;
    pid_t wpid;

    // Wait for all child processes to finish
    while ((wpid = wait(&status)) > 0)
    {
        if (WIFEXITED(status))
        {
            printf("Child process %d (PID: %d) exited with status %d\n", wpid, getpid(), WEXITSTATUS(status));
        }
        else
        {
            printf("Child process %d (PID: %d) terminated abnormally\n", wpid, getpid());
        }
    }

    return 0;
}

/////////////////////////////
//                              DEFINE FUNCTIONS
/////////////////////////////

// Create the child procces, modify the parameters and lounch the executable
void create_child_proccess(const char *filename)
{
    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        perror("Counldn't create child process");
    }
    if (pid == 0)
    {
        printf("Child proccess PID %d\n", getpid());

        char    *exec_name;

        exec_name=analise_config(filename);

        execl(exec_name, (const char*)exec_name, (char*)NULL);
        exit(EXIT_SUCCESS);
    }
    if (pid > 0)
    {
        printf("Parent proccess PID %d\n", getpid());
    }
}

void launch_group(const char *filename)
{
    FILE    *file;
    int     n=1;
    char    line[64];

    file=fopen(filename, "r");
    fgets(line,64,file);

    if(strstr(line,"[group:"))
    {
        fscanf(file,"%d",&n);
    }
    //TODO: for -> efery prog in 

    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        perror("Counldn't create child process");
    }
    if (pid == 0)
    {
        printf("Child proccess PID %d\n", getpid());

        char    *exec_name;

        exec_name=analise_config(filename);

        execl(exec_name, (const char*)exec_name, (char*)NULL);
        exit(EXIT_SUCCESS);
    }
    if (pid > 0)
    {
        printf("Parent proccess PID %d\n", getpid());
    }
}

// redirects the stdin, stdout, stderr to the specified file
void redirect(char *parameters, int fd_redirected)
{
    char copy[50];
    strcpy(copy, parameters);

    char *filename = strtok(copy, " :");
    int flags = (int)(fd_redirected >= 1);
    char *flag = strtok(NULL, " \n\0");

    //printf("%s\n",flag);

    while (flag)
    {
        if (strcmp(flag, "create") == 0)
        {
            flags |= O_CREAT;
        }
        else if (strcmp(flag, "truncate") == 0)
        {
            flags |= O_TRUNC;
        }
        else if (strcmp(flag, "append") == 0)
        {
            flags |= O_APPEND;
        }
        flag = strtok(NULL, " \n\0");
    }
    int fd_out = open(filename, flags, 0644);
    dup2(fd_out, fd_redirected);
    close(fd_out);
}

// change the owners of the proccess, first the group_owner, lather the owner
// if we change first the user then it may not have enought permitions to change the group_owner
// this function has to be called right before the exec command otherwise the rest of the parameter changes may not be done
void change_owner(uid_t uid)
{
    if (setuid(uid) != 0)
    {
        perror("Error setting UID");
        exit(EXIT_FAILURE);
    }
}

void change_gowner(gid_t gid)
{
    if (setgid(gid) != 0)
    {
        perror("Error setting GID");
        exit(EXIT_FAILURE);
    }
}

// change the priority of the proccess
void change_priority(int new_priority)
{
    int which = PRIO_PROCESS; // Adjust the priority of a process
    id_t pid = getpid();

    if (setpriority(which, pid, new_priority) == -1)
    {
        perror("Error setting process priority");
        exit(EXIT_FAILURE);
    }
}

void change_umask(mode_t new_umask)
{
    umask(new_umask);
}

void change_exec_directory(char *path_to_directory)
{
    if (chdir(path_to_directory) != 0)
    {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }
}

int key_word(char* word)
{
    if(strcmp(word,"stdin") == 0)
    {
        return W_STDIN;
    } else
    if(strcmp(word,"stdout") == 0)
    {
        return W_STDOUT;
    } else
    if(strcmp(word,"stderr") == 0)
    {
        return W_STDERR;
    } else
    if(strcmp(word,"owner") == 0)
    {
        return W_OWNER;
    } else
    if(strcmp(word,"group_owner") == 0)
    {
        return W_GOWNER;
    } else 
    if(strcmp(word,"umask") == 0)
    {
        return W_UMASK;
    } else
    if(strcmp(word,"priority") == 0)
    {
        return W_PRIORITY;
    } else
    if(strcmp(word,"directory") == 0)
    {
        return W_DIR;
    }
    else 
        return -1;

}

int get_line(FILE*file)
{
    int     n=0;
    char    line[200];
    while (fgets(line,200,file)) {
        n++;
    }
    fseek(file,0,SEEK_SET);
    return n;
}

int compare_settings(const void *a, const void *b)
{
    return ((Setting*)a)->set_w-((Setting*)b)->set_w;
}

void _erase_front_spaces(char** string)
{
    while((*string)[0]==' ')
    {
        (*string)=(*string)+1;
    }
}

char* analise_config(const char *filename)
{
    FILE    *file;
    int     n;
    char    line[64];
    char    *word;
    char    *path_to_exec;
    Setting *settings;

    file=fopen(filename, "r");

    n=get_line(file);

    settings=(Setting*)calloc(n-2,sizeof(Setting));

    fgets(line,64,file);
    fgets(line,64,file);

    path_to_exec=(char*)malloc(strlen(line)*sizeof(char));
    strcpy(path_to_exec,strtok(line,"\n"));
 /*  TODO:
    //verify program/group/include
 */
    for(int i=0;i<n-2;i++) {

        fgets(line,64,file);
        word=strtok(line,": ");
        settings[i].set_w=key_word(word);
        if(settings[i].set_w==-1)
        {
            printf("Incorect key owrd\n");
            i--;
            n--;
        }
        word=strtok(NULL,"\n");
        strcpy(settings[i].parameters,word);
    }

    qsort(settings, n-2, sizeof(Setting), compare_settings);

    for(int i=0;i<n-2;i++)
    {
        char *copy=(char*)malloc((strlen(settings[i].parameters)+1)*sizeof(char));
        switch (settings[i].set_w)
        {
        case W_STDIN:
            redirect(settings[i].parameters,STDIN_FILENO);
            break;
        case W_STDOUT:
            redirect(settings[i].parameters,STDOUT_FILENO);
            break;
        case W_STDERR:
            redirect(settings[i].parameters,STDERR_FILENO);
            break;
        case W_DIR:
            strcpy(copy,settings[i].parameters);
            _erase_front_spaces(&copy);
            strcpy(settings[i].parameters,copy);

            change_exec_directory(settings[i].parameters);
            break;
        case W_PRIORITY:
            int priority;

            strcpy(copy,settings[i].parameters);
            _erase_front_spaces(&copy);
            strcpy(settings[i].parameters,copy);
            priority=atoi(settings[i].parameters);
            change_priority(priority);
            break;
        case W_UMASK:
            int   mask;

            strcpy(copy,settings[i].parameters);
            _erase_front_spaces(&copy);
            strcpy(settings[i].parameters,copy);
            mask=strtol(settings[i].parameters, NULL, 8);
            change_umask(mask);
            break;
        case W_GOWNER:
            strcpy(copy,settings[i].parameters);
            _erase_front_spaces(&copy);
            strcpy(settings[i].parameters,copy);

            struct group    *grp_info = getgrnam(settings[i].parameters);

            if (grp_info == NULL) {
                //perror("Error getting group information");
                return NULL;
            }
            change_gowner(grp_info->gr_gid);
        case W_OWNER:
            strcpy(settings[i].parameters,strtok(settings[i].parameters," "));
            struct passwd   *pwd_info = getpwnam(settings[i].parameters);

            if (pwd_info == NULL) {
                perror("Error getting user information");
                return NULL;
            }
            change_owner(pwd_info->pw_uid);

        default:
            break;
        }
    }

    return path_to_exec;
}

void* communicate_fsup(void *arg)
{
    printf("In comm with fsup\n");

    int sockfd, new_sockfd;
    socklen_t clilen;
    struct sockaddr_un serv_addr, cli_addr;
    pthread_t thread_id;

    // Create a socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCK_PATH, sizeof(serv_addr.sun_path) - 1);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server waiting for connections...\n");
    fflush(stdout);

    while (1) {
        // Accept a connection
        clilen = sizeof(cli_addr);
        new_sockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (new_sockfd == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create a thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_sockfd) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to allow it to clean up automatically
        pthread_detach(thread_id);
    }

    // Close the server socket (this part may not be reached due to the infinite loop)
    close(sockfd);

    return NULL;

}

void* handle_client(void *arg)
{
    int client_fd = *((int *)arg);
    char buffer[256];

    ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[bytesRead] = '\0';  // Null-terminate the received data
    printf("Server received: %s\n", buffer);

    strcpy(buffer,"Bidirectional comm achieved!\n");

    bytesRead = write(client_fd, buffer, strlen(buffer) - 1);
    if (bytesRead == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Close the client socket
    close(client_fd);

    return NULL;
}

void* communicate_manager(void *arg)
{
    int conn_fd;
    Connection_t* conn_param = (Connection_t*)arg;

    __uint16_t port = 0;
    int rc = sscanf(conn_param->port, "%hu", &port);
    DIE(rc != 1, "invalid port");

    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    //TODO: uncommend, jurnalizare, rese conditions, mutes pt scrierea in fisier
    
    /* rc = inet_pton(AF_INET, conn_param->ip, &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect()");

    printf("Connected to the manager...\n"); */
}