#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>	

#include "tcp_ip_lib.h"
#include "intermud.h"

int sock;
int connect_mode = NO_CONNECT;
int server_sock;

/* broken pipe signal handler */
void sig_pipe(int param)
{
	close(sock);
	signal(SIGPIPE, sig_pipe);
}

/* alarm signal handler, used when connecting */
void sig_alarm(int param)
{
	close(server_sock);
	signal(SIGALRM, sig_alarm);
}


int main(int argc, char *argv[])
{

	unsigned long count = 0;
	char buf[256];
	int num_read;
	char in_buf[5];	
	fd_set afds, rfds;
	/* int nfds = 0; */
	int alen;
	struct sockaddr_in fsin;
	struct timeval timeout;
	static char welcome[] = "\n\r\n\rWelcome to Intermud server version 1.0\n\r\n\r"; 
	
	signal(SIGALRM, sig_alarm);
	signal(SIGPIPE, sig_pipe);
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

/* 	server_sock = passiveTCP(PORT, QUEUE); */
	while(1)
	{
		/* wait to see if a client connection occurs */
		if (!connect_mode)
		{

			/* interrupt the accept call after 5 seconds
       * and attempt to connect as a client */
#if 0 
			FD_ZERO(&afds);
			FD_SET(server_sock, &afds);
			printf("Calling select\n\r");
			if (select(64, &afds, 0, 0, &timeout) < 0)
			{
				printf("Select error\n\r");
				exit(0);
			}
			if (FD_ISSET(server_sock, &afds))
			{
				sock = accept(server_sock, (struct sockaddr *)&fsin, &alen);
				if (sock < 0)
				{
					if (errno == EINTR) 
						continue;
					printf("Accept error\n\r");
				}
				connect_mode = SERVER;
				write(sock, welcome, strlen(welcome));
				printf("I am the server\n\r");
			}
#endif
		}

		/* wait a bit and try to connect to the other mud as a client */
		sleep(2);
		if (!connect_mode)
		{
			/* attempt a client connection */
			printf("Attempting client connection\n\r");
			sock = connectTCP(SERVER_HOSTNAME, PORT);
			if (sock < 0)
				printf("Can't connect to %s\n\r", SERVER_HOSTNAME);
			else
			{
				printf("I am the client\n\r");
				connect_mode = CLIENT;
			}
		}

		sleep(2);

		/* the two muds are now connected so begin the main loop */
		while(connect_mode)
		{
			sleep(1);
			switch(connect_mode)
			{
				case SERVER:
					sprintf(buf, "Server Idle %ld\n", count++);
					if (write(sock, buf, strlen(buf)) < 0)
					{
						printf("Write error, closing socket\n\r");
						close(sock);
						connect_mode = NO_CONNECT;
					}
					/* read stuff from the socket */
					FD_ZERO(&rfds);
					FD_SET(sock, &rfds);
					select(64, &rfds, 0, 0, &timeout);
					if (FD_ISSET(sock, &rfds))
					{
						if ((num_read = read(sock, in_buf, sizeof(in_buf)-1)) < 0)
						{
							close(sock);
							connect_mode = NO_CONNECT;
						}
						else
						{
							in_buf[num_read] = '\0';
							printf("%s", in_buf);
						}
					}
					break;

				case CLIENT:
					sprintf(buf, "Client Idle %ld\n", count++);
					if (write(sock, buf, strlen(buf)) < 0)
					{
						printf("Write error, closing socket\n\r");
						close(sock);
						connect_mode = NO_CONNECT;
					}
					/* read stuff from the socket */
					FD_ZERO(&rfds);
					FD_SET(sock, &rfds);
					select(64, &rfds, 0, 0, &timeout);
					if (FD_ISSET(sock, &rfds))
					{
						if ((num_read = read(sock, in_buf, sizeof(in_buf))) < 0)
						{
							close(sock);
							connect_mode = NO_CONNECT;
						}
						else
						{
							in_buf[num_read] = '\0';
							printf("%s", in_buf);
						}
					}
					break;

				default:
					printf("Illegal connect mode\n\r");
					exit(0);
			}
		}
		printf("Connection closed\n\r");
		close(sock);	 
	}
}
