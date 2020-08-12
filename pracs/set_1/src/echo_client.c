/*
 * A UDP client that sends a string to a server and waits for a response.
 *
 * Compile with: cc src/echo_client.c -o build/echo_client -std=c89
 *
 * Usage: build/echo_client server_name port_number string
 * Example: ./build/echo_client 127.0.0.1 1800 "test string"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argsc, char *argsv[])
{
        int csd;                        /* client socket descriptor */
        struct hostent *host;           /* pointer to host details returned by
                                              gethostbyname()*/
        struct sockadd_in server;       /* server strucutre. contains family, 
                                           port, in_addr struct, zero byte */
        int server_size;                /* Size of server structure */
        int string_len;                 /* Size of the string sent to the server.
                                              Includes trailing null. */

        short server_port_num;
        int out_cnt, in_cnt;            /* byte counts for send and receive */

        /* variables to hold strings sent and received */
        char client_string[BUF_LEN];
        char response_string[BUF_LEN];

        /* Handle command line arguments */
        if (argsc != 4) 
        {
                fprintf(stderr, "Missing 1 or more arguments. 
                                Expected: server_addr port_num string");
                exit(EXIT_FAILURE);
        }

        /* resovle the server host name or handle error */
        server_host = gethostbyname(argsv[1]);
        
        if (server_host == NULL)
        {
                herror("While calling gethostbyname()");
                exit(EXIT_FAILURE);
        }

        /* Parse port string and copy the send_string */
        server_port_num = atoi(argv[2]);
        strcpy(client_string, argv[3]);

        csd = socket(PF_INET, SOCK_DGRAM, 0);

        if (csd < 0)
        {
                perror("While calling socket()");
                exit(EXIT_FAILURE);
        }

        server.sin_family = AF_INET;
        /* copy data from host details returned by
        memcpy(&server.sin_addr.s_addr, server_host->h_addr_list[0],
                        server_host->h_length);
}
