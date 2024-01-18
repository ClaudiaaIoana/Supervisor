all: backsup test1 fsup

backsup: backsup.c
	gcc -o backsup ../utils/json.c backsup.c -pthread

fsup: fsup.c
	gcc -o fsup ../utils/json.c fsup.c -pthread

IP_SERVER = 127.0.0.1
PORT = 12345

run_backsup:
	rm -rf /tmp/mysocket
	./backsup ${IP_SERVER} ${PORT}

run_fsup_sleep:
	./fsup -e sleep 