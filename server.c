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

// get sockaddr in either IPv4 or IPv6
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
    int sockfd, new_fd;

    char buf[MAXDATASIZE];

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr; // connector's address information
    struct sigaction sa;
    
    socklen_t sin_size;
    
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    char *port;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (argv[1] == NULL)
    {
        fprintf(stderr, "Please provide a valid port number\n");
        return 1;
    }
    else
    {
        port = argv[1];
    }

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
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
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap dead child processes created from fork()
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
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

            printf("server: sending output of the following command to client: %s\n", buf);

            dup2(new_fd, 1); // output
            dup2(new_fd, 2); // errors

            // Parsing for use with execvp()
            char *token;
            int arg_count = 0;
            char *args[256];

            token = strtok(buf, " ");
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

    freeaddrinfo(servinfo);

    close(sockfd);

    return 0;
}