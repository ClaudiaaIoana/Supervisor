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

#define SOCK_PATH "/tmp/mysocket"


int main()
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
	char *message = "Hello, server!";
    ssize_t bytesSent = write(sockfd, message, strlen(message));
    if (bytesSent == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    //get data from server
    int bytesRead = read(sockfd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    printf("%s\n",buffer);

		message = "Hello, server! 0";
    for(int i=0;i<5;i++)
    {
		sleep(1);
		ssize_t bytesSent = write(sockfd, message, strlen(message));
		if (bytesSent == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		message[strlen(message)-1]+=i;
    }

    // Close the socket
    close(sockfd);

    return 0;

    return 0;
}

