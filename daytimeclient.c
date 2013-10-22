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

int tcp_connect(const char *host)
{
	int connfd;
	struct sockaddr_in servaddr;

	connfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connfd == -1)
		error(EXIT_FAILURE, errno, "socket");

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(IPPORT_DAYTIME);
	if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
		fprintf(stderr, "incorrection ip address format\n");	
		exit(EXIT_FAILURE);
	}
	if (connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
		error(EXIT_FAILURE, errno, "connect");

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
		connfd = tcp_connect(host);	
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
