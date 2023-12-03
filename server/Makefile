CC=	gcc
CFLAGS = -Wall -Wextra -g

server: server.c
	$(CC) $(CFLAGS) server.c -o server 

client: client.c
	$(CC) $(CFLAGS) client.c -o client 

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza clientul 	
run_client:
	./client ${IP_SERVER} ${PORT}

.PHONY clean:
	rm -rf server