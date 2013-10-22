#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>

int tcp_connect(const char *host, const char *service)
{
	int connfd;
	struct addrinfo hints, *res, *rp;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));	
		exit(EXIT_FAILURE);
	}

	for (rp = res; rp; rp = rp->ai_next) {
		connfd = socket(rp->ai_family, rp->ai_socktype, 
			        rp->ai_protocol);
		if (connfd == -1)
			continue;
		if (connect(connfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(connfd);
	}

	if (rp == NULL)	{
		fprintf(stderr, "couldn't connect %s\n", host);	
		exit(EXIT_FAILURE);
	}

	return connfd;
}

void time_recv(int connfd)
{
	ssize_t rlen;
	char timestr[64];

	rlen = read(connfd, timestr, sizeof(timestr));
	if (rlen == -1)
		error(EXIT_FAILURE, errno, "read");	
	else
		timestr[rlen] = 0;
	close(connfd);
}

void test_concur(const char *host)
{
	int connfd;

	/* 
	 * when execute connect in a very short time, it ocurrs error
	 * cannot assign requested address. because tcp connection
	 * is in TIME_WAIT status.
	 * revolution:
	 *	enable kernel support for tcp timestamp
	 * 	sysctl -w net.ipv4.tcp_timestamps=1 
	 *	enable fast retrive of tcp connection in TIME_WAIT
	 *	sysctl -w net.ipv4.tcp_tw_recycle=1
	 */
	while (1) {
		connfd = tcp_connect(host, "daytime");	
		time_recv(connfd);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <host ip>\n", argv[0]);	
		exit(EXIT_FAILURE);
	}

	test_concur(argv[1]);

	exit(0);
}
