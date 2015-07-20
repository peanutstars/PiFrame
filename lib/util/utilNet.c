
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pflimit.h"
#include "pfdebug.h"

/*****************************************************************************/

int PFU_isValidStrIPv4 (const char *strIp)
{
	int ret = 0;
	unsigned int ip[4];

	if ( strIp && 4 == sscanf(strIp, "%u.%u.%u.%u", ip, ip+1, ip+2, ip+3) )
	{
//		DBG("strIp=%s parseIp=%u.%u.%u.%u\n", strIp, ip[0], ip[1], ip[2], ip[3]);
		do
		{
			if ( ip[0] & ~0xFF || ip[1] & ~0xFF ||
				 ip[2] & ~0xFF || ip[3] & ~0xFF ) {
				break;
			}
			ret = 1;
		}
		while (0);
	}
	return ret;
}

int PFU_openTCP (const char *strIp, int port, int fgNonBlock, int timeout/*sec*/)
{
	int sock = -1; 
	struct sockaddr_in server_addr;
	struct hostent *server;
	int socketType = SOCK_STREAM | (fgNonBlock ? SOCK_NONBLOCK : 0);
	int waitCount = 0;
	int rv = 0;

	sock = socket (AF_INET, socketType, 0); 
	if (sock < 0) {   
		rv = -errno;
		ERR2("socket()\n");
		return rv;
	}   

	server = gethostbyname(strIp);
	if (server == NULL) {
		rv = -errno;
		ERR2("gethostbyname(%s)\n", strIp);
		close (sock);
		return rv;
	}
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
	server_addr.sin_port = htons(port);

	do  
	{   int timeTick = timeout * 2;
		if (fgNonBlock && waitCount > timeTick) {
			rv = -ETIMEDOUT;
			/* This funciton is using to check a validate system for other device on board */
			ERR("Connect(Timeout over %d second)\n", timeout);
			break;
		}
		if ( connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ) {   
			rv = -errno;
			if (fgNonBlock) {
				if (errno == EINPROGRESS || errno == EALREADY) {
					waitCount ++;
					nanosleep((struct timespec[]){{0, 500000000}}, NULL);
					continue;
				}
				if (errno == EINTR) {
					continue;
				}
			}
			ERR2 ("connect(%s:%d)\n", strIp, port);
			break;
		}   
		return sock;
	}   
	while(fgNonBlock);

	if (sock >= 0) {
		close (sock);
	}
	return rv;
}

