#include "server.h"

int main() {

    server my_server;
    my_server.init();
    my_server.accept_connections();
    return 0;
}