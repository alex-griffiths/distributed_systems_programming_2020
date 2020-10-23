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
void manage_request(int, int);
void response(char* verb, char* document, char* version, char* out_buf);
void random_lines(char *poem_buffer, int num_lines);
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

                fprintf(stderr, "[ SERVER ] Accepted connection from %s "\
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
                        manage_request(es_sd, es_sd);
                        
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
                        fprintf(stderr, "[ SERVER ] Process %d will serve this.\n", pid);
                }
        }

        close(rs_sd);
}

void manage_request(int in, int out) 
{
        int read_count, buf_count, out_count;
        char out_buf[BUF_LEN],
             in_data[BUF_LEN],
             hostname[40];
        char prefix[100];
        char *req_word; /* A token from the HTTP request */

        int token_counter;
        char rand_buf[BUF_LEN];

        char *verb;
        char *document;
        char *version;

        /* Seed random number gen */
        srand(time(NULL));

        gethostname(hostname, 40);

        /* Output the process id for each child that is handling a connection */
        sprintf(prefix, "\t    Managing process [ %d ]:", getpid());
        fprintf(stderr, "\n%s starting up\n", prefix);

        buf_count = 0;
        memset(in_data, '\0', BUF_LEN * sizeof(char));

        read_count = read(in, in_data + buf_count, COM_BUF_LEN);
        if (read_count < 0) 
        {
                perror("While calling read()");
                exit(EXIT_FAILURE);
        }

        fprintf(stderr, "%s\r\n", in_data);

        memset(verb, '\0', 4);
        if (in_data[0] == 'H')
        {
                /* allococate memory for verb and then set it */
                verb = (char *)malloc(sizeof(char) * 4);
                if (verb == NULL) 
                {
                        perror("While calling malloc()");
                        exit(EXIT_FAILURE);
                }
                sprintf(verb, "%s", "HEAD");
        } else if (in_data[0] == 'G') 
        {
                /* allococate memory for verb and then set it */
                verb = (char *)malloc(sizeof(char) * 3);
                if (verb == NULL) 
                {
                        perror("While calling malloc()");
                        exit(EXIT_FAILURE);
                }
                sprintf(verb, "%s", "GET");
        }


        /* Parse the reqest to get the verb. Split request into individula words */
        req_word = strtok(in_data, " ");
        token_counter = 0; 

        while (req_word != NULL)
        {
                /* Check the request is for the document root */
                if (token_counter == 1) 
                {
                        /* Allocate space for the document string */
                        document = (char *)malloc(sizeof(char) * strlen(req_word));
                        if (document != NULL) 
                        {
                                sprintf(document, "%s", req_word);
                        }
                        else
                        {
                                perror("While calling malloc()");
                                exit(EXIT_FAILURE);
                        }
                } 
                else if (token_counter == 2)
                {
                        /* Allocate space for the document string */
                        version = (char *)malloc(sizeof(char) * 8);
                        if (version != NULL) 
                        {
                                strncpy(version, req_word, 8);
                        }
                        else
                        {
                                perror("While calling malloc()");
                                exit(EXIT_FAILURE);
                        }
                } 
                else if (token_counter > 2)
                {
                        break;
                }

                token_counter++;
                req_word = strtok(NULL, " ");
        }

        response(verb, document, version, out_buf);

        out_count = write(out, out_buf, strlen(out_buf));
        if (out_count < 0)
        {
                perror("WHile calling write()");
                exit(EXIT_FAILURE);
        }

        fprintf(stderr, "\n%s Closing down\n", prefix);
        close(in);
}

/* Creates a response to a request and writes it to out_buf. */
void response(char *verb, char *document, char *version, char *out_buf) 
{
        char *poem_buffer;
        char *cl_line; /* Content length line */
        int num_lines;

        fprintf(stderr, "VERB: %s\r\n", verb);
        fprintf(stderr, "DOC: %s\r\n", document);
        fprintf(stderr, "VERSION: %s\r\n", version);

        /* Clear response buffer */
        memset(out_buf, '\0', BUF_LEN * sizeof(char));

        /* Get random number of lines from poem */
        num_lines = rand() % 51;

        /* Aproximate the space required for the poem_buffer */
        poem_buffer = (char *)malloc(sizeof(char) * 100 * num_lines);
        random_lines(poem_buffer, num_lines);

        /* Make sure we're only handling HEAD and GET requests */
        if (strcmp(verb, "HEAD") == 0 || strcmp(verb, "GET") == 0) 
        {
                if (strcmp(document, "/") == 0)
                {
                        /* Write to output buffer */

                        /* Status Line */
                        sprintf(out_buf, "%s 200 OK\r\n", version);

                        /* Response headers */
                        /* Content-Type */
                        strcat(out_buf, "Content-Type: text/plain; charset=UTF-8\r\n");

                        /* Content-Length */
                        sprintf(cl_line, "Content-Length:%d\r\n\r\n", 
                                (int)strlen(poem_buffer));
                        strcat(out_buf, cl_line);

                        if (strcmp(verb, "GET") == 0)
                        {
                                /* Message body */
                                strcat(out_buf, poem_buffer);
                        }

                } else {
                        /* Return 404 */
                        sprintf(out_buf, "%s 404 Not Found\r\n", version);
                        /* Content-Type */
                        strcat(out_buf, "Content-Type: text/plain; charset=UTF-8\r\n");
                        /* Content-Length */
                        strcat(out_buf, "Content-Length:0\r\n\r\n");
                }
        } else {
                /* Return 501 */
                sprintf(out_buf, "%s 501 Not Implemented\r\n", version);
                /* Content-Type */
                strcat(out_buf, "Content-Type: text/plain; charset=UTF-8\r\n");
                /* Content-Length */
                strcat(out_buf, "Content-Length:0\r\n\r\n");
        }

        fprintf(stderr, "%s", out_buf);
}

void random_lines(char *poem_buffer, int num_lines) 
{
        char *line_buffer;
        char poem[] =   "Gay go up, and gay go down,\r\n"
                        "To ring the bells of London town.\r\n"
                        " \r\n"
                        "Bull's eyes and targets,\r\n"
                        "Say the bells of St. Margret's.\r\n"
                        " \r\n"
                        "Brickbats and tiles,\r\n"
                        "Say the bells of St. Giles'.\r\n"
                        " \r\n"
                        "Halfpence and farthings,\r\n"
                        "Say the bells of St. Martin's.\r\n"
                        " \r\n"
                        "Oranges and lemons,\r\n"
                        "Say the bells of St. Clement's.\r\n"
                        " \r\n"
                        "Pancakes and fritters,\r\n"
                        "Say the bells of St. Peter's.\r\n"
                        " \r\n"
                        "Two sticks and an apple,\r\n"
                        "Say the bells at Whitechapel.\r\n"
                        " \r\n"
                        "Pokers and tongs,\r\n"
                        "Say the bells at St. John's.\r\n"
                        " \r\n"
                        "Kettles and pans,\r\n"
                        "Say the bells at St. Ann's.\r\n"
                        " \r\n"
                        "Old Father Baldpate,\r\n"
                        "Say the slow bells at Aldgate.\r\n"
                        " \r\n"
                        "Maids in white Aprons\r\n"
                        "Say the bells of St Catherine's.\r\n"
                        " \r\n"
                        "You owe me ten shillings,\r\n"
                        "Say the bells of St. Helen's.\r\n"
                        " \r\n"
                        "When will you pay me?\r\n"
                        "Say the bells at Old Bailey.\r\n"
                        " \r\n"
                        "When I grow rich,\r\n"
                        "Say the bells at Shoreditch.\r\n"
                        " \r\n"
                        "Pray when will that be?\r\n"
                        "Say the bells of Stepney.\r\n"
                        " \r\n"
                        "I'm sure I don't know,\r\n"
                        "Says the great bell at Bow.\r\n"
                        " \r\n"
                        "Here comes a candle to light you to bed,\r\n"
                        "And here comes a chopper to chop off your head.";

        /* split poem into lines. Copy the random number of lines into a buffer*/
        line_buffer = strtok(poem, "\r\n");
        while (line_buffer != NULL && num_lines > 0)
        {
                strcat(poem_buffer, line_buffer);
                strcat(poem_buffer, "\r\n");

                num_lines--;
                line_buffer = strtok(NULL, "\r\n");
        }
}


void handle_sig_child(int sig_num)
{
        pid_t child;

        while (0 < waitpid(-1, NULL, WNOHANG));
}
