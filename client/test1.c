#include <stdio.h>
#include <unistd.h>

int main() {
    int uid, euid;
    int gid, egid;

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

    return 0;
}
