#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>

int main() {
    printf("Gonna sleep a lot\n");
    usleep(100000000);

    return 0;
}