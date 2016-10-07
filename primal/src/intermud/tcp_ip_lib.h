#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <varargs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef INADDR_NONE
  #define INADDR_NONE	0xffffffff
#endif

extern int 	errno;
extern char 	*sys_errlist[];

u_short htons();
u_long	inet_addr();
/* u_short portbase = 0; */


int connectTCP(char *host, char *service );
int connectUDP(char *host, char *service );
int connectsock( char *host, char *service, char *protocol );
int passiveTCP( char *service, int qlen );
int passiveUDP( char *service );
int passivesock( char *service, char *protocol, int qlen );
/* int errexit(char *format, va_dcl va_alist); */

int errexit(char *message);
