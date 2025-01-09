#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096
#define MAXDATASIZE 100

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int sendall(int sockfd, const char *buf, size_t len)
{
    size_t total = 0;        // total bytes sent
    size_t bytes_left = len; // bytes left to send
    ssize_t n;

    while (total < len)
    {
        n = send(sockfd, buf + total, bytes_left, 0);
        if (n == -1)
        {
            perror("send");
            break;
        }
        total += n;
        bytes_left -= n;
    }

    return n == -1 ? -1 : 0;
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s hostname [port]\n", argv[0]);
        exit(1);
    }

    const char *hostname = argv[1];
    const char *port = (argc == 3) ? argv[2] : "80";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);

    char request[1024];
    snprintf(request, sizeof request,
             "GET / HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             hostname);

    if (sendall(sockfd, request, strlen(request)) == -1)
    {
        fprintf(stderr, "Failed to send all bytes\n");
        close(sockfd);
        return 1;
    }

    printf("HTTP request sent:\n%s\n", request);

    int numbytes;
    printf("HTTP response:\n");
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) > 0)
    {
        buf[numbytes] = '\0';
        printf("%s", buf);
    }
    if (numbytes == -1)
    {
        perror("recv");
    }

    close(sockfd);
    return 0;
}
