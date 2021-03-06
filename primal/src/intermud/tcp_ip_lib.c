#include "tcp_ip_lib.h"


u_short portbase = 0;

/* connectTCP.c - connectTCP */

/*------------------------------------------------------------------------
 * connectTCP - connect to a specified TCP service on a specified host
 *------------------------------------------------------------------------
 */
int
connectTCP( host, service )
char	*host;		/* name of host to which connection is desired	*/
char	*service;	/* service associated with the desired port	*/
{
	return connectsock( host, service, "tcp");
}
/* connectUDP.c - connectUDP */

/*------------------------------------------------------------------------
 * connectUDP - connect to a specified UDP service on a specified host
 *------------------------------------------------------------------------
 */
int
connectUDP( host, service )
char	*host;		/* name of host to which connection is desired	*/
char	*service;	/* service associated with the desired port	*/
{
	return connectsock(host, service, "udp");
}
/* connectsock.c - connectsock */


/*------------------------------------------------------------------------
 * connectsock - allocate & connect a socket using TCP or UDP
 *------------------------------------------------------------------------
 */
int
connectsock( host, service, protocol )
char	*host;		/* name of host to which connection is desired	*/
char	*service;	/* service associated with the desired port	*/
char	*protocol;	/* name of protocol to use ("tcp" or "udp")	*/
{
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/


	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;

    /* Map service name to port number */
	if ( (pse = getservbyname(service, protocol)) )
		sin.sin_port = pse->s_port;
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		/* errexit("can't get \"%s\" service entry\n", service); */
                 errexit("Can't get service entry");   

    /* Map host name to IP address, allowing for dotted decimal */
	if ( (phe = gethostbyname(host)) )
		bcopy(phe->h_addr, (char *)&sin.sin_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		/* errexit("can't get \"%s\" host entry\n", host); */
                 errexit("Can't get host entry");

    /* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(protocol)) == 0)
		/* errexit("can't get \"%s\" protocol entry\n", protocol); */
		errexit("Can't get protocol entry");

    /* Use protocol to choose a socket type */
	if (strcmp(protocol, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

    /* Allocate a socket */

	/* s = socket(PF_INET, type, ppe->p_proto); */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
{
		/* errexit("can't create socket: %s\n", sys_errlist[errno]); */
		printf("%s   ", sys_errlist[errno]);
		errexit("Can't create socket");
}
    /* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		/* errexit("can't connect to %s.%s: %s\n", host, service,
			sys_errlist[errno]); */
		return(errexit("Can't connect to host"));
	return s;
}
/* passiveTCP.c - passiveTCP */

/*------------------------------------------------------------------------
 * passiveTCP - create a passive socket for use in a TCP server
 *------------------------------------------------------------------------
 */
int
passiveTCP( service, qlen )
char	*service;	/* service associated with the desired port	*/
int	qlen;		/* maximum server request queue length		*/
{
	return passivesock(service, "tcp", qlen);
}
/* passiveUDP.c - passiveUDP */

/*------------------------------------------------------------------------
 * passiveUDP - create a passive socket for use in a UDP server
 *------------------------------------------------------------------------
 */
int
passiveUDP( service )
char	*service;	/* service associated with the desired port	*/
{
	return passivesock(service, "udp", 0);
}
/* passivesock.c - passivesock */

/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *------------------------------------------------------------------------
 */
int
passivesock( service, protocol, qlen )
char	*service;	/* service associated with the desired port	*/
char	*protocol;	/* name of protocol to use ("tcp" or "udp")	*/
int	qlen;		/* maximum length of the server request queue	*/
{
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

    /* Map service name to port number */
	if ( (pse = getservbyname(service, protocol)) )
		sin.sin_port = htons(ntohs((u_short)pse->s_port)
			+ portbase);
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		/* errexit("can't get \"%s\" service entry\n", service); */
		errexit("Can't get service entry");

    /* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(protocol)) == 0)
		/* errexit("can't get \"%s\" protocol entry\n", protocol); */
		errexit("Can't get protocol entry");

    /* Use protocol to choose a socket type */
	if (strcmp(protocol, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

    /* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0)
		/* errexit("can't create socket: %s\n", sys_errlist[errno]); */
		errexit("Can't create socket");


    /* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		/* errexit("can't bind to %s port: %s\n", service, 
			sys_errlist[errno]); */
		errexit("Can't bind to the port");
	if (type == SOCK_STREAM && listen(s, qlen) < 0)
		/* errexit("can't listen on %s port: %s\n", service,
			sys_errlist[errno]); */
		errexit("Can't listen on the port");
	return s;
}
/* errexit.c - errexit */


/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */

/* int errexit(format, va_alist)
char	*format;
va_dcl
{
	va_list	args;

	va_start(args);
	_doprnt(format, args, stderr); 
        printf("Error\n");
	va_end(args);
	exit(1); 
}
*/

int errexit(char *message)
{
	printf("%s\n", message);
/* 	exit(1); */
	return(-1);
}
