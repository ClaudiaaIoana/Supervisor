CC=gcc
CFLAGS= -Wall -Wextra -g

all: backsup test1
backsup: backsup.c
	$(CC) $(CFLAGS) ../utils/json.c backsup.c -o server 


PORT = 12345
IP_SERVER = 127.0.0.1

run_backsup:
	./server ${IP_SERVER} ${PORT}