#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
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
#include <stdbool.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>

#include "../utils/utils.h"
#include "../utils/json.h"

#define SOCK_PATH "/tmp/mysocket"
int sockfd_fsup_comm;

#define MAX_PATH_LENGTH 256
#define MAX_WORD_LENGTH 50
#define CONF_PATH "../configure"

// when working with pipes
#define READ_END 0
#define WRITE_END 1

// when working with standard fd
#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

//key words
#define W_NUMBER    8
#define W_DIR       1
#define W_STDIN     2
#define W_STDOUT    3
#define W_STDERR    4
#define W_PRIORITY  5
#define W_UMASK     6
#define W_GOWNER    7
#define W_OWNER     8

typedef enum Operations{
    EXEC,
    BLOCK,
    CONTINUE,
    KILL
} Operations;

typedef enum Commands {
    EXEC_COMMAND,
    BLOCK_COMMAND,
    CONTINUE_COMMAND,
    KILL_COMMAND
} Commands;

struct Setting{
    int     set_w;
    char    parameters[200];

}typedef Setting;

struct Connection{
    char ip[16];
    char port[6];
}typedef Connection_t;

//MUTEX FOR ACCESSING THE PROCS
pthread_mutex_t mutex_pr;

pthread_mutex_t mutex_log;

int sockfd_manager = 0;

FILE* log=NULL;

struct Proc{
    pid_t pid;
    char conf[50];
    struct Proc* next;
    bool status;
}typedef Proc_ch;

int num_proc=0;
Proc_ch *procs=NULL;

void add_proc(pid_t pid, char* conf);

void remove_proc(pid_t pid);

Proc_ch *get_proc(pid_t pid);

int create_child_proccess(char *exec_name, Setting *settings);

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

void set_initial_conf(Setting *settings, char *line);

char* analise_config(char *filename, Setting *settings, bool *flag);

char *get_conf_file(char* directory_path, const char* target_word);

void apply_settings(Setting *settings);

void* communicate_fsup(void *arg);

void* handle_client(void *arg);

void* communicate_manager(void *arg);

void handle_forced_exit(int signum);

int isProcessAlive(pid_t pid);

pid_t* list_child_processes(int *n);

void write_message_log(char* message);

void write_word(char* word);

char* get_time();

/////////////////////////////////////////// MAIN

int main(int argc, char* argv[])
{
    struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

    pthread_mutex_init(&mutex_pr, NULL);
    pthread_mutex_init(&mutex_log, NULL);
 
	sa.sa_flags = SA_RESETHAND;
	sa.sa_handler = handle_forced_exit;
	sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

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

    return 0;
}

/////////////////////////////
//                              DEFINE FUNCTIONS
/////////////////////////////

// Create the child procces, modify the parameters and lounch the executable
int create_child_proccess(char *exec_name, Setting *settings)
{
    pid_t pid;
    char pid_s[10];
    pid = fork();
    if (pid < 0)
    {
        perror("Counldn't create child process");
    }
    if (pid == 0)
    {
        printf("Child proccess PID %d\n", getpid());

        pthread_mutex_lock(&mutex_log);
            log=fopen("activity.log","a");
            fprintf(log, "<%s> created child process %d\n", get_time(),getpid());
            fclose(log);
            log=NULL;
        pthread_mutex_unlock(&mutex_log);

        apply_settings(settings);

        execl(exec_name, (const char*)exec_name, (char*)NULL);
        exit(EXIT_SUCCESS);
    }
    if (pid > 0)
    {
        printf("Parent proccess PID %d\n", getpid());
        add_proc(pid,exec_name);
        printf("Out of add\n");
        return pid;
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

        //exec_name=analise_config(filename);

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

void set_initial_conf(Setting * settings, char *line)
{
    char    copy[250];
    char    *p;

    strcpy(copy,line);
    p=strtok(copy," ");
    p=strtok(NULL," ");
    p=strtok(NULL," ");

    pid_t pid = atoi(p);
    if (kill(pid, SIGSTOP) == -1) {
        perror("kill");
    }

    p=strtok(NULL," ");

    settings[W_OWNER-1].set_w=W_OWNER;
    strcpy(settings[W_OWNER-1].parameters,p);

    p=strtok(NULL," ");
    settings[W_GOWNER-1].set_w=W_GOWNER;
    strcpy(settings[W_GOWNER-1].parameters,p);

    p=strtok(NULL," ");
    settings[W_STDIN-1].set_w=W_STDIN;
    strcpy(settings[W_STDIN-1].parameters,p);

    p=strtok(NULL," ");
    settings[W_STDOUT-1].set_w=W_STDOUT;
    strcpy(settings[W_STDOUT-1].parameters,p);

}

char* analise_config(char *filename, Setting *settings, bool *flag)
{
    FILE    *file;
    int     n;
    int     read_lines = 0;
    char    line[250];
    char    *word;
    char    *path_to_exec=NULL;


    file=fopen(filename, "r");

    n=get_line(file);

    fgets(line,250,file); 

    if(strstr(line,"[program:")!=NULL)
    {
        fgets(line,250,file);
        path_to_exec=(char*)malloc(strlen(line)*sizeof(char));
        strcpy(path_to_exec,strtok(line,"\n"));
        read_lines=2;
    } else 
    if(strstr(line,"[group:")!=NULL)
    {
        fgets(line,250,file);
        fgets(line,250,file);
        read_lines=3;
    }

    for(int i=0;i<n-read_lines;i++) {
        
        fgets(line,250,file);
        word=strtok(line,": ");

        int key=key_word(word);

        if(key==-1)
        {
            printf("Incorect key word: %s\n", word);
            continue;
        } else 
        if(key == W_STDIN || key == W_STDOUT || key == W_STDERR)
        {
            (*flag)=true;
        }
        settings[key-1].set_w=key;
        word=strtok(NULL,"\n");
        strcpy(settings[key-1].parameters,word);
    }
    return path_to_exec;
}

char *get_conf_file(char* directory_path, const char* target_word)
{
	DIR *dir;
    struct dirent *entry;
    FILE *file;
    char file_path[MAX_PATH_LENGTH];
    char buffer[MAX_WORD_LENGTH];

    // Open the directory
    if ((dir = opendir(directory_path)) == NULL) {
        perror("opendir");
        return NULL;
    }

    // Iterate over entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path of the file
        snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", directory_path, entry->d_name);

        // Open the file
        if ((file = fopen(file_path, "r")) == NULL) {
            perror("fopen");
            continue; // Skip to the next file if unable to open
        }

        // Read the file line by line
        if (fgets(buffer, MAX_WORD_LENGTH, file) != NULL) {
            // Check if the target word is in the line
            if (strstr(buffer, target_word) != NULL) {
                // Close the file and directory
                fclose(file);
                closedir(dir);

                return strdup(file_path);  // Return a duplicate of the file path
            }
        }
        // Close the file
        fclose(file);
    }

    // Close the directory
    closedir(dir);

    // If no match found, return NULL
    return NULL;
}

void apply_settings(Setting * settings)
{
    for(int i=0;i<W_NUMBER;i++)
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
                perror("Error getting group information");
            }
            change_gowner(grp_info->gr_gid);
        case W_OWNER:
            strcpy(settings[i].parameters,strtok(settings[i].parameters," "));
            struct passwd   *pwd_info = getpwnam(settings[i].parameters);

            if (pwd_info == NULL) {
                perror("Error getting user information");
            }
            change_owner(pwd_info->pw_uid);

        default:
            break;
        }
    }
}

void* communicate_fsup(void *arg)
{
    int                 new_sockfd;
    socklen_t           clilen;
    struct sockaddr_un  serv_addr, cli_addr;
    pthread_t           thread_id;

    // Create a socket
    sockfd_fsup_comm = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd_fsup_comm == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;

    if (setsockopt(sockfd_fsup_comm, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(sockfd_fsup_comm);
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCK_PATH, sizeof(serv_addr.sun_path) - 1);

    // Bind the socket
    if (bind(sockfd_fsup_comm, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");
        close(sockfd_fsup_comm);
        exit(EXIT_FAILURE);
    }

    if (chmod(SOCK_PATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
    perror("chmod");
    close(sockfd_fsup_comm);
    exit(EXIT_FAILURE);
}

    // Listen for incoming connections
    if (listen(sockfd_fsup_comm, 5) == -1) {
        perror("listen");
        close(sockfd_fsup_comm);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server waiting for connections...\n");
    fflush(stdout);

    while (1) {
        // Accept a connection
        clilen = sizeof(cli_addr);
        new_sockfd = accept(sockfd_fsup_comm, (struct sockaddr*)&cli_addr, &clilen);
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
    close(sockfd_fsup_comm);

    return NULL;

}

void* handle_client(void *arg)
{
    int     client_fd = *((int *)arg);
    char    buffer[250];
    char    file_name[250];
    char    *p;
    char    *pid_fsup_ch;
    char    line[250];
    char    *path_to_exec;
    pid_t   pid;
    pid_t   pid_fsup;
    bool    flag_redir=false;
    Setting *settings;

    ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
	  perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[bytesRead] = '\0';
    char    copy[250];
    strcpy(copy,buffer);

    p = strtok(copy," ");
    if(strcmp(p,"exec")==0)
    {
        p=strtok(NULL," ");
        strcpy(file_name,p);
        pid_fsup_ch = strtok(NULL," ");
        pid_fsup = atoi(pid_fsup_ch);


        FILE*   file = fopen(file_name,"r");

        fgets(line,250,file);
        
        if(strstr(line,"[program:")!=NULL)
        {
            printf("-----------------------------\n");
            fclose(file);
            settings = (Setting *)calloc(W_NUMBER, sizeof(Setting));
            set_initial_conf(settings, buffer);
            path_to_exec = analise_config(file_name,settings, &flag_redir);
            pid = create_child_proccess(path_to_exec, settings);
            free(path_to_exec);
            free(settings);
            
        } else 
        if(strstr(line,"[group:")!=NULL)
        {
            //printf("In group\n");

            printf("-----------------------------\n");
            fgets(line,250,file);
            
            int     nr = atoi(line);
            char    *p;
            char    **programs = (char**)calloc(nr,sizeof(char*));

            fgets(line,250,file);
            fclose(file);
            p=strtok(line, " ");

            for (int i = 0; i < nr; i++) {
                programs[i] = (char*)calloc(strlen(p)+1,sizeof(char));
                strcpy(programs[i],p);
                p=strtok(NULL," \n\t");
            }

            bool    prev_flag = flag_redir;

            for (int i = 0; i < nr; i++) {
                settings = (Setting *)calloc(W_NUMBER, sizeof(Setting));

                strcpy(copy,buffer);
                set_initial_conf(settings,copy);
                path_to_exec = analise_config(get_conf_file(CONF_PATH,programs[i]), settings, &flag_redir);
                analise_config(file_name, settings, &flag_redir);
                int     current_pid = create_child_proccess(path_to_exec, settings);
                if(flag_redir == true && prev_flag==false)
                {
                    pid = current_pid;
                }
                prev_flag=flag_redir;
                free(programs[i]);
                free(path_to_exec);
                free(settings);
            }
            free(programs);
        }


        if(!flag_redir) {
            int status;
            if(waitpid(pid,&status,0) == -1)
            {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            remove_proc(pid);
        }

        if (kill(pid_fsup, SIGCONT) == -1)
            perror("kill fsup");

        if(flag_redir) {
            int status;
            if(waitpid(pid,&status,0) == -1)
            {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            remove_proc(pid);
        }
        
    }
    else if(strcmp(p,"procs")==0)
    {
        char out[64];
        char owner[64];
        p=strtok(NULL," ");
        strcpy(out,p);
        pid_fsup_ch = strtok(NULL," ");
        pid_fsup = atoi(pid_fsup_ch);
        p=strtok(NULL," ");
        strcpy(owner,p);

        if (kill(pid_fsup, SIGSTOP) == -1) 
            perror("kill");

        int num;
        pid_t *pids=list_child_processes(&num);

        FILE*f=fopen(out,"w");

        for(int i=0;i<num;i++)
        {
            fprintf(f,"%d\n",pids[i]);
        }

        fclose(f);

        if (kill(pid_fsup, SIGCONT) == -1)
            perror("kill fsup");

        free(pids);
    }else 
    if(strcmp(p,"det_procs")==0)
    {
        char            out[64];
        char            owner[64];
        struct passwd   *pw;
        int num;

        p=strtok(NULL," ");
        strcpy(out,p);
        pid_fsup_ch = strtok(NULL," ");
        pid_fsup = atoi(pid_fsup_ch);
        p=strtok(NULL," ");
        strcpy(owner,p);

        if (kill(pid_fsup, SIGSTOP) == -1) 
            perror("kill");

        pid_t *pids=list_child_processes(&num);

        FILE*f=fopen(out,"w");

        fprintf(f,"PID\tSTATUS\tEXEC\n");
        for(int i=0;i<num;i++)
        {
            fprintf(f,"%d\t",pids[i]);
             if (get_proc(pids[i])->status)
                fprintf(f, "R\t");
            else
                fprintf(f, "B\t");

            // Print the configuration information for the process
            Proc_ch *aux = procs;
            while (aux)
            {
                if (aux->pid == pids[i])
                {
                    fprintf(f, "%s\n", aux->conf);
                    break;
                }
                aux = aux->next;
            }
        }

        fclose(f);

        if (kill(pid_fsup, SIGCONT) == -1)
            perror("kill fsup");

        free(pids);
        
    }
    else if(strcmp(p,"procs")==0)
    {
        char out[64];
        char owner[64];
        p=strtok(NULL," ");
        strcpy(out,p);
        pid_fsup_ch = strtok(NULL," ");
        pid_fsup = atoi(pid_fsup_ch);
        p=strtok(NULL," ");
        strcpy(owner,p);

        if (kill(pid_fsup, SIGSTOP) == -1) 
            perror("kill");

        int num;
        pid_t *pids=list_child_processes(&num);

        FILE*f=fopen(out,"w");

        for(int i=0;i<num;i++)
        {
            fprintf(f,"%d\n",pids[i]);
        }

        fclose(f);

        if (kill(pid_fsup, SIGCONT) == -1)
            perror("kill fsup");

        free(pids);
    }
    else if(strcmp(p,"block")==0)
    {
        p=strtok(NULL," ");
        pid_t pid_ch=atoi(p);
        if (kill(pid_ch, SIGSTOP) == -1) 
            perror("block");
        Proc_ch *pr = get_proc(pid_ch);
        if(pr)
        {
            pr->status=false;
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "operation", BLOCK);
            cJSON_AddStringToObject(json, "status", "blocked");
            cJSON_AddNumberToObject(json, "pid", pid_ch);
            char *json_string = cJSON_Print(json);
            int rc = send(sockfd_manager, json_string, strlen(json_string), 0);
            DIE(rc < 0, "send()");
        }
    }
    else if(strcmp(p,"continue")==0)
    {
        p=strtok(NULL," ");
        pid_t pid_ch=atoi(p);
        if (kill(pid_ch, SIGCONT) == -1) 
            perror("continue");
        Proc_ch *pr = get_proc(pid_ch);
        if(pr)
        {
            pr->status=true;
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "operation", CONTINUE);
            cJSON_AddStringToObject(json, "status", "resumed");
            cJSON_AddNumberToObject(json, "pid", pid_ch);
            char *json_string = cJSON_Print(json);
            int rc = send(sockfd_manager, json_string, strlen(json_string), 0);
            DIE(rc < 0, "send()");
        }
    }
    else if(strcmp(p,"kill")==0)
    {
        p=strtok(NULL," ");
        pid_t pid_ch=atoi(p);
        if (kill(pid_ch, SIGTERM) == -1) 
            perror("kill");

        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "operation", KILL);
        cJSON_AddStringToObject(json, "status", "killed");
        cJSON_AddNumberToObject(json, "pid", pid_ch);
        char *json_string = cJSON_Print(json);
        int rc = send(sockfd_manager, json_string, strlen(json_string), 0);
        DIE(rc < 0, "send()");
    }

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
    
    rc = inet_pton(AF_INET, conn_param->ip, &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    rc = connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect()");

    printf("Connected to the manager...\n"); 

    sockfd_manager = conn_fd;

    char buffer[1024];
    while(1) {
        int rc = read(sockfd_manager, buffer, sizeof(buffer));

        if (rc == 0) {
            printf("The manager has closed the connection\n");
            break;
        }
        if (rc < 0) {
            perror("read");
            break;
        }

        buffer[rc] = '\0';
        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
            break;
        }
        Commands command = cJSON_GetObjectItem(json, "command")->valueint;
        switch(command) {
            case EXEC_COMMAND: {
                char *exec_name = cJSON_GetObjectItem(json, "exec_name")->valuestring;
                char *conf = cJSON_GetObjectItem(json, "conf")->valuestring;
                pid_t pid = create_child_proccess(exec_name, conf);
                break;
            }
            case BLOCK_COMMAND: {
                pid_t pid = cJSON_GetObjectItem(json, "pid")->valueint;
                break;
            }

            case CONTINUE_COMMAND: {
                pid_t pid = cJSON_GetObjectItem(json, "pid")->valueint;
                break;
            }

            case KILL_COMMAND: {
                pid_t pid = cJSON_GetObjectItem(json, "pid")->valueint;
                break;
            }

            default:
                break;
        }
    }
}

void handle_forced_exit(int signum)
{
    close(sockfd_fsup_comm);
    remove(SOCK_PATH);
    pthread_mutex_destroy(&mutex_pr);
    exit(0);
}

int isProcessAlive(pid_t pid) {
    return kill(pid, 0) != -1;
}

pid_t* list_child_processes(int *n) 
{
    pid_t pids[100];
    pid_t *_pids;
    int i=0;
    pthread_mutex_lock(&mutex_pr);
 
    Proc_ch *aux=procs;
    while(aux)
    {
        pids[i++]=aux->pid;
        aux=aux->next;
    }
    
    pthread_mutex_unlock(&mutex_pr);
    (*n)=i;
    _pids=(pid_t*)malloc(sizeof(pid_t)*i);
    for(i=0;i<(*n);i++)
    {
        _pids[i]=pids[i];
    }
    return _pids;
}

void add_proc(pid_t pid, char* conf)
{
    pthread_mutex_lock(&mutex_pr);
    if(!procs)
    {
        procs=(Proc_ch*)malloc(sizeof(Proc_ch));
        procs->pid=pid;
        strcpy(procs->conf,conf);
        procs->next=NULL;
        procs->status=true;
        pthread_mutex_unlock(&mutex_pr);
        return;
    }
    Proc_ch *new=(Proc_ch*)malloc(sizeof(Proc_ch));
    new->pid=pid;
    strcpy(new->conf,conf);
    new->next=procs;
    new->status=true;
    procs=new;
    num_proc++;
    pthread_mutex_unlock(&mutex_pr);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "operation", EXEC);
    cJSON_AddStringToObject(json, "status", "started");
    cJSON_AddNumberToObject(json, "pid", pid);
    char *json_string = cJSON_Print(json);
    int rc = send(sockfd_manager, json_string, strlen(json_string), 0);
    DIE(rc < 0, "send()");
}

void remove_proc(pid_t pid)
{
    pthread_mutex_lock(&mutex_pr);
    Proc_ch *aux=procs;
    Proc_ch *prev=NULL;
    while(aux)
    {
        if(aux->pid==pid)
        {
            if(!prev)
            {
                procs=aux->next;
                free(aux);
                num_proc--;
                pthread_mutex_unlock(&mutex_pr);
                return;
            }
            else{
                prev->next=aux->next;
                free(aux);
                num_proc--;
                pthread_mutex_unlock(&mutex_pr);
                return;
            }
        }
        prev=aux;
        aux=aux->next;
    }
}

Proc_ch *get_proc(pid_t pid)
{
    pthread_mutex_lock(&mutex_pr);
    Proc_ch *aux=procs;
    while(aux)
    {
        if(aux->pid==pid)
        {
            pthread_mutex_unlock(&mutex_pr);
            return aux;
        }
        aux=aux->next;
    }
    pthread_mutex_unlock(&mutex_pr);
    return NULL;
}

void write_message_log(char* message)
{
    time_t rawtime;
    struct tm *info;
    char timestamp[20];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(timestamp, sizeof(timestamp), "<%Y-%m-%d %H:%M> ", info);

    pthread_mutex_lock(&mutex_log);
    // Write the formatted date and message to the log
    fprintf(log, "%s %s", timestamp, message);
    pthread_mutex_unlock(&mutex_log);
}

void write_word(char* word)
{
    pthread_mutex_lock(&mutex_log);
    fprintf(log, " %s ", word);
    pthread_mutex_unlock(&mutex_log);
}

char* get_time()
{
    time_t rawtime;
    struct tm *info;
    char timestamp[20];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(timestamp, sizeof(timestamp), "<%Y-%m-%d %H:%M> ", info);

    return strdup(timestamp);
}

