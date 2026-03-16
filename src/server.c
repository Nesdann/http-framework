#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_client(int client_fd) {
    char buf[4096];
    memset(buf, 0, sizeof(buf));

    ssize_t bytes_read = read(client_fd, buf, sizeof(buf) - 1);
    if (bytes_read <= 0) {
        return;
    }

    printf("Received:\n%s\n", buf);

    // send a raw HTTP response
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, world!";

    write(client_fd, response, strlen(response));
}

int main() {
    //crate socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;



    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;


    //create bind, socket and listen
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
}


    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    printf("Server socket created successfully. %d\n", server_fd);
    if (listen(server_fd, 128) < 0) {
    perror("listen");
    exit(1);
    }

    printf("Server listening on port 8080\n");

    while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, 
                          (struct sockaddr*)&client_addr, 
                          &client_len);
    if (client_fd < 0) {
        perror("accept");
        continue;
    }

    // handle the client
    handle_client(client_fd);

    close(client_fd);
}

    return 0;
}