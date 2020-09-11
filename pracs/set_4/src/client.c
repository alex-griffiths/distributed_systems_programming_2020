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
        char *res_line;                 /* A line of a HTTP response */
        char *line_word;                 /* A line of a HTTP response */
        char *entity_field;             /* A field of the entity header or entity body */

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

        /* Capture user input and send it to the server until eod found */
        /*fgets(client_str, BUF_LEN, stdin);*/

        client_str_len = strlen(client_str);

        /* Make sure we've got an extra new line to match the expected format. */
        /*strncat(client_str, "\r\n", 2);*/

        /* Send request to server */
        out_count = write(child_sd, "HEAD / HTTP/1.0\r\n\r\n", strlen("HEAD / HTTP/1.0\r\n\r\n"));
        if (out_count < 0)
        {
                perror("While calling write()");
                exit(EXIT_FAILURE);
        }

        /* Clear server res buffer */
        memset(server_res, '\0', BUF_LEN * sizeof(char));

        /* Keep reading the response from the server until we reach the end. */
        while(read(child_sd, server_res, BUF_LEN -1) != 0)
        {
                res_line = strtok(server_res, "\r\n");

                while (res_line != NULL)
                {
                        if (strstr(res_line, "HTTP/1."))
                        {
                                fprintf(stderr, "%s\n", res_line);
                        }
                        if (strstr(res_line, "Content-Type:"))
                        {
                                fprintf(stderr, "%s\n", res_line);
                        }
                        if (strstr(res_line, "Last-Modified"))
                        {
                                fprintf(stderr, "%s\n", res_line);
                        }

                        res_line = strtok(NULL, "\r\n");
                }
                /*while (res_line != NULL)
                {
                        memset(line_word, '\0', sizeof(line_word));
                        strcpy(line_word, res_line);
                        line_word = strtok(line_word, " ");
                        while(line_word != NULL) 
                        {
                                if (strcmp(line_word, "HTTP/1.0") == 0)
                                {
                                        fprintf(stderr, "%s\r\n", res_line);
                                }

                                line_word = strtok(NULL, " ");
                        }

                        res_line = strtok(NULL, "\r\n");
                }*/

                /* Clear server res buffer */
                memset(server_res, '\0', BUF_LEN * sizeof(char));
        }

        fprintf(stderr, "Exiting...\n");
        close(child_sd);
        return 0;
}
