CC=gcc
CFLAGS = -Wall -Wextra -g

server: server.c
	$(CC) $(CFLAGS) ../utils/json.c server.c -o server -pthread

client: client.c
	$(CC) $(CFLAGS) ../utils/json.c client.c -o client 

PORT = 12345

IP_SERVER = 127.0.0.1

run_server:
	./server ${IP_SERVER} ${PORT}
 	
run_client:
	./client ${IP_SERVER} ${PORT}

.PHONY clean:
	rm -rf server