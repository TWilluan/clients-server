#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/****************************************
 * Clients - connect to specific server.
 *              Send a whois command and
 *              recieve output from server
*****************************************/

#define MAXDATASIZE 100

// get sockaddr in either IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    char *port;
    char *host;

    char *msg;
    int msg_len;

    if (argc < 3)
    {
        fprintf(stderr, "usage: client hostname:server_port whois [option] argument-list\n");
        return -1;
    }

    if (strcmp(argv[2], "whois") != 0)
    {
        fprintf(stderr, "Internal error: the command is not supported\n");
        return -1;
    }

    // Assign port and host
    char *ptrArg = argv[1];
    char *token;
    token = strtok(ptrArg, ":");
    host = token;
    token = strtok(NULL, ":");
    port = token;

    if (host == NULL || port == NULL)
    {
        fprintf(stderr, "Invalid hostname or port number\n");
        return 1;
    }

    // configure message to send
    msg = strdup(argv[2]);
    int i;
    for (i = 3; i < argc; i++)
    {
        msg = realloc(msg, strlen(msg) + strlen(argv[i]) + 1);
        strcat(msg, " ");
        strcat(msg, argv[i]);
    }

    msg_len = strlen(msg);

    // set up socket
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int status;
    char s[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // loop through all the results and connect to the first we can
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

    // send message to server
    int bytes_sent;
    if ((bytes_sent = send(sockfd, msg, msg_len, 0)) == -1)
    {
        perror("send");
        exit(1);
    }

    freeaddrinfo(servinfo);

    while (1)
    {
        numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);

        if (numbytes == 0)
        {
            exit(0);
        }

        if (numbytes == -1)
        {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("%s", buf);
    }

    free(msg);

    close(sockfd);

    return 0;
}