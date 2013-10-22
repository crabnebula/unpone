#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

int listenfd;

static void show_addrinfo(struct addrinfo *ai)
{
	assert(ai != NULL);
	in_port_t port;

	printf("flags: 0x%08x\n", ai->ai_flags);
	printf("family: %s\n",
		ai->ai_family == AF_INET ? "ipv4" :
		ai->ai_family == AF_INET6 ? "ipv6" : "unknown");
	printf("socktype: %s\n",
		ai->ai_socktype == SOCK_STREAM ? "tcp" : 
		ai->ai_socktype == SOCK_DGRAM ? "udp" : "other");
	printf("protocol: %d\n", ai->ai_protocol);
	printf("addrlen: %d\n", ai->ai_addrlen);
	char ipstr[INET6_ADDRSTRLEN];
	if (ai->ai_family == AF_INET) {
		inet_ntop(ai->ai_family,
			  &((struct sockaddr_in *)(ai->ai_addr))->sin_addr,
			  ipstr, sizeof(ipstr));
		port = ntohs(((struct sockaddr_in *)(ai->ai_addr))->sin_port);
	} else if (ai->ai_family == AF_INET6) {
		inet_ntop(ai->ai_family,
			  &((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr,
			  ipstr, sizeof(ipstr));
		port = ntohs(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_port);
	} else {
		ipstr[0] = 0;
	}
	printf("addr: %s:%d\n", ipstr, port);
	printf("canoname: %s\n", ai->ai_canonname);
}

int tcp_listen(void)
{
	int listenfd;
	int value = 1;
	struct addrinfo hints, *res, *rp;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(NULL, "daytime", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		exit(EXIT_FAILURE);
	}

	for (rp = res; rp; rp = rp->ai_next) {
		listenfd = socket(rp->ai_family, rp->ai_socktype,
				  rp->ai_protocol);
		if (listenfd == -1)
			continue;

		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &value,
			   sizeof(value));
		if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(listenfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "couldn't bind address\n");
		exit(EXIT_FAILURE);
	}

	if (listen(listenfd, 4) == -1)
		error(EXIT_FAILURE, errno, "listen");

	return listenfd;
}

static void *proc_routine(void *arg)
{
	time_t tval;
	char timestr[32];
	int connfd = *(int *)arg;
	int wlen = 0;

	tval = time(0);
	wlen = snprintf(timestr, sizeof(timestr), "%.24s\r\n", ctime(&tval));
	if (write(connfd, timestr, wlen) != wlen)
		error(0, errno, "write");
	close(connfd);
	free(arg);
	pthread_exit(NULL);
}

static int connect_process(int connfd)
{
	pthread_t tid;
	pthread_attr_t attr;
	int ret, *fdp;

	fdp = malloc(sizeof(connfd));
	if (fdp == NULL) {
		error(0, errno, "malloc");	
		return -1;
	}
	*fdp = connfd;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&tid, &attr, proc_routine, fdp);
	if (ret != 0)
		error(0, ret, "pthread_create");
	pthread_attr_destroy(&attr);
	
	return 0;
}

int dtserv_run(int listenfd)
{
	int connfd;
	struct sockaddr_in clitaddr;
	socklen_t addrlen;
	time_t oldsec, cursec;
	int count = 0;
	fd_set rfds;
	int ret;
	struct timeval tv;

	oldsec = time(0);
	for ( ;; ) {
		FD_ZERO(&rfds);
		FD_SET(listenfd, &rfds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		ret = select(listenfd+1, &rfds, NULL, NULL, &tv);
		if (ret == -1) {
			error(0, errno, "select");
			continue;
		} else if (ret == 0)
			continue;
		if (!FD_ISSET(listenfd, &rfds))
			continue;
		addrlen = sizeof(clitaddr);
		connfd = accept(listenfd, (struct sockaddr *)&clitaddr,
				&addrlen);
		if (connfd == -1) {
			error(0, errno, "accept");
			continue;
		}
		connect_process(connfd);	
		count++;
		cursec = time(0);
		if (cursec > oldsec) {
			printf("last sec qps: %d\n", count);		
			count = 0;
			oldsec = cursec;
		}
	}

}

static void sig_handler(int signo)
{
	close(listenfd);
	exit(0);
}

void signal_setup(void)
{
	struct sigaction sa;

	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
}

int main(int argc, char *argv[])
{
	signal_setup();
	listenfd = tcp_listen();
	dtserv_run(listenfd);

	exit(0);
}
