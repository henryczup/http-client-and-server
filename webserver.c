#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BACKLOG 10
#define DEFAULTPORT "28333"

int main(int argc, char *argv[])
{
    if (argc < 1 || argc > 2)
    {
        fprintf(stderr, "usage: %s [port]", argv[0]);
        exit(1);
    }

    const char *port = (argc == 2) ? argv[1] : DEFAULTPORT;

    int rv;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return 1;
    }

    int sockfd, new_fd;
    int yes = 1;
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("set socket");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("bind");
            exit(1);
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        perror("failed to bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1)
    {
        perror("accept");
        close(sockfd);
        exit(1);
    }

    printf("Connection accepted from client\n");

    char buf[1024];
    char request[1024];
    int totalrecv, bytesrecv = 0;
    while ((bytesrecv = recv(new_fd, buf, 1024 - 1, 0)) > 0)
    {
        buf[bytesrecv] = "\0";
        strcat(request, buf);
        totalrecv += bytesrecv;

        if (strstr(request, "\r\n\r\n") != NULL)
        {
            break;
        }
    }

    if (bytesrecv == -1)
    {
        perror("receive");
        close(new_fd);
        close(sockfd);
        exit(1);
    }

    printf("Received message: %s\n", buf);

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";

    send(new_fd, response, strlen(response), 0);

    close(sockfd);
    close(new_fd);

    return 0;
}