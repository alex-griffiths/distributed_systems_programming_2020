/*
 * Alex Griffiths - 18001525
 *
 * A concurrent connection oriented server.
 *
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
#include <time.h>

#define TCP_PORT 1800

#define BUF_LEN 512
#define COM_BUF_LEN 32

void manage_connection(int, int);
int server_processing(char *in_str, char *out_str);
void handle_sig_child(int);

int main() 
{
        int rs_sd;                         /* Rendezvous socket socket descriptor */
        int es_sd;                         /* empheral socket socket descriptor */
        int error_code;
        int client_len;
        int pid;

        struct sockaddr_in server, client; /* address structures for server and
                                              clients */
        struct hostent *client_details;
        struct sigaction child_sig;        /* used to handle SIGCHLD to prevent
                                              zombies */

        /* Seed random number gen */
        srand(time(NULL));

        fprintf(stderr, "[ SERVER ] THE SERVER IS STARTING...\n");

        /* 
         * Setup the SIGCHLD handling in order to handle any zombie child
         * processes.
         */

        child_sig.sa_handler = handle_sig_child;
        sigfillset(&child_sig.sa_mask);
        child_sig.sa_flags = SA_RESTART | SA_NOCLDSTOP; 
        sigaction(SIGCHLD, &child_sig, NULL);

        /* Create a stream oriented rendezvous socket */
        rs_sd = socket(PF_INET, SOCK_STREAM, 0);
        if (rs_sd < 0) 
        {
                perror("[ SERVER ] While creating rendezvous socket");
                exit(EXIT_FAILURE);
        }

        /*
         * Set up server address details to bind them to a specified socket.
         * It should use the TCP/IP address family.
         * Use INADDR_ANNY so that it can recieve messages sent to any of
         * its interfaces.
         * It needs to convert the port number from host to network order.
         * htonl -> For hostlong
         * htons -> For hostshort
         */

        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(TCP_PORT);

        /* Bind the socket */
        if ((error_code = bind(rs_sd, (struct sockaddr *) &server, sizeof(server))) < 0) 
        {
                perror("[ SERVER ] While binding the socket");
                exit(EXIT_FAILURE);
        }

        fprintf(stderr, "[ SERVER ]\t Bind complete\n");

        /* Passively get connections with accept(). Limit to 7 connections */
        if ((error_code = listen(rs_sd, 5)) < 0)
        {
                perror("[ SERVER ] While setting the listen queue up");
                exit(EXIT_FAILURE);
        }

        fprintf(stderr, "[ SERVER ]\t listening\n");
        fprintf(stderr, "[ SERVER ] ... Setup complete. Ready to accept connections\n");

        /*
         * Keep wiating for clients to connect.
         * As clients connect, pick them off the queue established by listen()
         * with accept()
         *
         * accept() will return a socket descriptor that can be used to 
         * communicate with the client.
         */

        while(1)
        {
                client_len = sizeof(client);
                if ((es_sd = accept(rs_sd, (struct sockaddr *) &client, 
                                                (socklen_t *) &client_len)) < 0)
                {
                        perror("[ SERVER ] While accepting a connection");
                        exit(EXIT_FAILURE);
                }

                /* Get the name of the client and print it. */
                client_details = gethostbyaddr((void *) &client.sin_addr.s_addr, 
                                         4, AF_INET);

                if (client_details == NULL)
                {
                        herror("[ SERVER ] While resolving the client's address");
                        exit(EXIT_FAILURE);
                }

                fprintf(stderr, "[ SERVER ] Accepted connection from %s"\
                                        "on port %d. es_sd = %d\n",
                                        client_details->h_name, 
                                        ntohs(client.sin_port),
                                        es_sd);

                /* Fork and handle incoming client */
                if ((pid = fork()) == 0)
                {
                        /*
                         * Close the rendezvous socket as it isn't needed in the
                         * child process. Pass processing off to manage_connection()
                         * function
                         */
                        close(rs_sd);
                        manage_connection(es_sd, es_sd);
                        
                        /* The client has been handled. Exit with success code */
                        exit(EXIT_SUCCESS);
                }
                else 
                {
                        /*
                         * Parent doens't need the descriptor from accept()
                         * so close it. If we don't, we will eventually run
                         * out of descriptors.
                         */

                        close(es_sd);
                        fprintf(stderr, "[ SERVER ] Process %d will server this.\n", pid);
                }
        }

        close(rs_sd);
}

void manage_connection(int in, int out)
{
        int read_count, buf_count, out_count;
        char out_buf[BUF_LEN],
             in_data[BUF_LEN],
             hostname[40];
        char prefix[100];

        char eod = '&'; /* termination char */
        int i;
        int rand_count;
        char rand_buf[BUF_LEN]; /* Buffer to hold randomly case toggle chars */

        gethostname(hostname, 40);
        /* Output the process id for each child that is handling a connection */
        sprintf(prefix, "\t   Managing process [ %d ]:", getpid());
        fprintf(stderr, "\n%s starting up\n", prefix);

        /* Announcement message sent to client on connection. */
        sprintf(out_buf, "\n\n Connected to concurrent connection-oriented server\n"\
                         "Host: %s\n Exit with a single 'X' character.\nTerminate strings"\
                         " with the '&' character.\n", hostname);

        write(out, out_buf, strlen(out_buf));

        /*
         * Continue reading until eod is reached, or the buffer runs out.
         *
         * An ampersand is used as a termination character.
         */

        while(1)
        {
                buf_count = 0;
                /* Clear the buffer */
                memset(in_data, '\0', BUF_LEN * sizeof(char));
                while(1)
                {
                        read_count = read(in, in_data + buf_count, COM_BUF_LEN);
                        if (read_count > 0)
                        {
                                /* Handle buffer overflow */
                                if ((buf_count + read_count) > BUF_LEN)
                                {
                                        /*
                                         * Here we just return an error.
                                         * normally we should split the buffer.
                                         */

                                        fprintf(stderr, "\n%s Buffer size exceeded",
                                                        prefix);
                                        close(in);
                                        exit(EXIT_FAILURE);
                                }
                                
                                /* Handle submitted buffer */
                                fprintf(stderr, "%s Recieved:\n", prefix);
                                for (i = buf_count; i < buf_count + read_count; i++)
                                {
                                        fprintf(stderr, "%s\t%c\n", prefix, 
                                                        in_data[i]);
                                }

                                buf_count = buf_count + read_count;

                                if (in_data[buf_count - 2] == eod) break;
                                if (in_data[buf_count - 3] == eod) break;
                        }
                        else if (read_count == 0)
                        {
                                fprintf(stderr, "\n%s Client has closed connection. Closing.\n",
                                                prefix);
                                close(in);
                                exit(EXIT_FAILURE);
                        }
                        else
                        {
                                sprintf(prefix, "%s: While reading from connection", prefix);
                                perror(prefix);
                                close(in);
                                exit(EXIT_FAILURE);
                        }
                }

                /* Remove new line chars off the end of the string. */
                if (in_data[0] == 'X') break;
                in_data[strlen(in_data) - 1] = '\0';

                rand_count = server_processing(in_data, rand_buf);

                /* Wait until eod is received before replying to client */
                sprintf(out_buf, "The server receieved %d characters, which"\
                                 " when the case is randomly toggled are:"\
                                 "\n%s\n\nEnter next string: ",
                                 (int)(strlen(rand_buf)), rand_buf);

                out_count = write(out, out_buf, strlen(out_buf));
                if (out_count < 0)
                {
                        perror("While calling write()");
                        exit(EXIT_FAILURE);
                }
        }

        fprintf(stderr, "\n%s Client has exited the session. Closing down\n", prefix);

        close(in);
}

int server_processing(char *in_str, char *out_str)
{
        int i, len;
        int r_num; /* Random number */
        char c;

        len = strlen(in_str);

        for(i = 0; i < len; i++)
        {
                r_num = rand();

                if (r_num % 2 == 0)
                {
                        out_str[i] = toupper(in_str[i]);
                }
                else
                {
                        out_str[i] = tolower(in_str[i]);
                }
        }

        out_str[len] = '\0';

        return len;
}

void handle_sig_child(int sig_num)
{
        pid_t child;

        while (0 < waitpid(-1, NULL, WNOHANG));
}
