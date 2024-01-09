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
#include <dirent.h>

#define SOCK_PATH "/tmp/mysocket"
#define CONF_PATH "../configure"

#define MAX_PATH_LENGTH 256
#define MAX_WORD_LENGTH 50

char *get_conf_file(const char* directory_path, const char* target_word);

char *make_exec_string(const char* target_word);

char *make_proc_string();

void connect_to_backsup();

int main(int argc, char* argv[])
{
    int opt;
    char* program =NULL;
    char to_send[250]={'\0'};

    while ((opt = getopt(argc, argv, "e:")) != -1) {
        switch (opt) {
            case 'e':
                strcpy(to_send,"exec ");
                program = optarg;
                strcat(to_send,make_exec_string(program));
                break;
            case '?':
                break;
        }
    }

    optind = 1;

    while ((opt = getopt(argc, argv, "p")) != -1) {
        printf("in get opt 2\n");
        switch (opt) {
            case 'p':
                strcat(to_send,"procs ");
                strcat(to_send,make_proc_string());
                break;
            case '?':
                break;
        }
    }

    connect_to_backsup(to_send);
    usleep(100000);
    return 0;
}

char *get_conf_file(const char* directory_path, const char* target_word)
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

void connect_to_backsup(char* message)
{
    int sockfd;
    char buffer[256];
    struct sockaddr_un serv_addr;

    // Create a socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCK_PATH, sizeof(serv_addr.sun_path) - 1);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Send data to the server
    ssize_t bytesSent = write(sockfd, message, strlen(message));
    if (bytesSent == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Close the socket
    close(sockfd);
}

char *make_exec_string(const char *target_word)
{
    char            *file;
    char            owner[50];
    char            gowner[50];
    uid_t           uid = geteuid();
    struct passwd   *pw;
    struct group    *grp;
    int             pid = getppid();
    char            pid_s[10];
    char            to_send[250];

    file=get_conf_file(CONF_PATH,target_word);
	//INPUT
    char *terminal_in = ttyname(0);
    //OUTPUT
    char* terminal_out = ttyname(1);
    // Get the user name
    pw = getpwuid(uid);
    strcpy(owner, pw->pw_name);
    gid_t gid = getgid();
    // Get the group name
    grp = getgrgid(gid);
    strcpy(gowner, grp->gr_name);
    //get pid
    sprintf(pid_s, "%d", pid);


    strcpy(to_send,file);
    strcat(to_send," ");
    strcat(to_send,pid_s);
    strcat(to_send," ");
    strcat(to_send,owner);
    strcat(to_send," ");
    strcat(to_send,gowner);
    strcat(to_send," ");
    strcat(to_send,terminal_in);
    strcat(to_send," ");
    strcat(to_send,terminal_out);

    return strdup(to_send);
}

char *make_proc_string()
{
    char            owner[50];
    uid_t           uid = geteuid();
    struct passwd   *pw;
    char            to_send[250];
    int             pid = getppid();
    char            pid_s[10];
    //OUTPUT
    char* terminal_out = ttyname(1);
    // Get the user name
    pw = getpwuid(uid);
    strcpy(owner, pw->pw_name);
    gid_t gid = getgid();
    //get pid
    sprintf(pid_s, "%d", pid);

    strcpy(to_send,terminal_out);
    strcat(to_send," ");
    strcat(to_send,pid_s);
    strcat(to_send," ");
    strcat(to_send,owner);

    return strdup(to_send);
}
