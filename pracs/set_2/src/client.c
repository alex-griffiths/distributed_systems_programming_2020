/*
 * Alex Griffiths - 18001525
 *
 * A connection oriented client.
 *
 * Compile: cc client.c -o client
 *
 * Usage: ./client Server Port
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#define BUF_LEN 512

int main(int argc, char *argv[])
{
        int child_sd;                   /* Child socket descriptor       */
        int con_val;                    /* Connection value              */
        struct sockaddr_in server;      /* Server address structure      */
        struct hostent *server_host;

        int server_len;                 /* size of server addr structure */
        int out_str_size;

        short server_port;              /* Server port num               */
        int out_count, in_count;        /* Byte counts for send/receive  */
        char client_str[BUF_LEN];
        int client_str_len;
        char server_res[BUF_LEN];

        /* Check cli args */
        if (argc != 3)
        {
                fprintf(stderr, "Usage: %s Server Port", argv[0]);
                exit(EXIT_FAILURE);
        }

        /* Parse arguments */
        /* Use resolver to get server address */
        server_host = gethostbyname(argv[1]);

        /* Make sure no issues with the server host */
        if (server_host == NULL)
        {
                herror("While calling gethostbyname()");
                exit(EXIT_FAILURE);
        }

        server_port = atoi(argv[2]);

        /* Create the socket */
        child_sd = socket(PF_INET, SOCK_STREAM, 0);

        /* Make sure no issues with socket */
        if (child_sd < 0)
        {
                perror("While calling socket()");
                exit(EXIT_FAILURE);
        }

        /* Prep server struct */
        server.sin_family = AF_INET;
        memcpy(&server.sin_addr.s_addr, server_host->h_addr_list[0],
                        server_host->h_length);
        server.sin_port = htons(server_port);
        server_len = sizeof(server);

        fprintf(stderr, "Making connection to remote server.\n");

        con_val = connect(child_sd, (struct sockaddr *)&server, server_len);
        if (con_val < 0) 
        {
                perror("While calling connect()");
                exit(EXIT_FAILURE);
        }

        /* Begin connection loop */
        while (1)
        {
                /* Clear server res buffer */
                memset(server_res, '\0', BUF_LEN * sizeof(char));

                in_count = read(child_sd, server_res, BUF_LEN);

                if (in_count < 0) 
                {
                        perror("While calling read()");
                        exit(EXIT_FAILURE);
                }
                else if (in_count == 0) 
                {
                        fprintf(stderr, "Connection to server lost\n");
                        break;
                }

                fprintf(stderr, "%s\n", server_res);

                /* Capture user input and send it to the server until eod found */
                while (1)
                {
                        fgets(client_str, BUF_LEN, stdin);

                        client_str_len = strlen(client_str);

                        out_count = write(child_sd, client_str, strlen(client_str));
                        if (out_count < 0) 
                        {
                                perror("While calling write()");
                                exit(EXIT_FAILURE);
                        }

                        /* Check for eod character or for exit char */
                        if (client_str[client_str_len - 2] == '&') break;
                }

                /* Check for exit char */
                if (client_str[0] == 'X') break;
        }

        fprintf(stderr, "Exiting...\n");
        close(child_sd);
        return 0;
}
