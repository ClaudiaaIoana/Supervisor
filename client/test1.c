#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>

int main() {
    int uid, euid;
    int gid, egid;
    char    buffer[50];

    scanf("%s",buffer);
    printf("BUFFER: %s ??",buffer);

    // Get real user and group IDs
    uid = getuid();
    gid = getgid();

    // Get effective user and group IDs
    euid = geteuid();
    egid = getegid();

    // Print the information
    printf("Real User ID: %d\n", uid);
    printf("Real Group ID: %d\n", gid);
    printf("Effective User ID: %d\n", euid);
    printf("Effective Group ID: %d\n", egid);

    int pid = getpid();  // Get the process ID of the current process

    int priority = getpriority(PRIO_PROCESS, pid);

    if (priority == -1) {
        perror("Error getting process priority");
        return 1;
    }

    printf("Process priority: %d\n", priority);

    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        return 1;
    }

    printf("Current working directory: %s\n", cwd);

    int mask=umask(0);

    printf("Current mask : %o\n",mask);

    return 0;
}
