#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/*********************************************
 *  Simple server design recieving and
 *      executing command from clients.
 *      Streaming output back to the client
**********************************************/

#define MAXDATASIZE 500
#define BACKLOG 20

void sigchld_handler(int s)
{
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
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
    // Initialize variables
    int sockfd, new_fd;
    
    int numbytes;
    int buf[MAXDATASIZE];

    struct addrinfo hints, *server_info, *p;
    struct sockaddr_storage client_addr;
    struct sigaction sa;

    socklen_t sin_size;

    char s[INET6_ADDRSTRLEN];
    char *port;
    int status;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (argv[1] == NULL)
    { //check if port is provided
        fprintf(stderr, "the port is NULL");
        return 1;
    } else 
        port = argv[1];

    if ((status = getaddrinfo(NULL, port, &hints, &server_info)) != 0)
    { //check if getaddrinfo is called succesful
        fprintf(stderr, "getaddressinfo: %s", gai_strerror(status));
        return 1;
    }


    for (p = server_info; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("server: setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "failed to bind\n");
        return 1;
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1)
    { // main accept() loop
        sin_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork())
        {
            close(sockfd);

            // Recieve from client
            int bytes_recv = recv(new_fd, buf, sizeof(buf), 0);
            buf[bytes_recv] = '\0';

            printf("server: sending output of the following command to client: %d\n", *buf);

            dup2(new_fd, 1); // output
            dup2(new_fd, 2); // errors

            char *token;
            int arg_count = 0;
            char *args[256];

            char *char_buf = (char *) buf; //convert buf to char* to avoid warning
            token = strtok(char_buf, " ");
            while (token != NULL)
            {
                args[arg_count] = token;
                arg_count++;
                token = strtok(NULL, " ");
            }

            args[arg_count] = NULL;

            char *const *args_const = (char *const *)args;

            execvp("whois", args_const);

            perror("execvp");
            exit(1);
        }

        close(new_fd);
    }

    freeaddrinfo(server_info);

    close(sockfd);

    return 0;
}