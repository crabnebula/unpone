#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static void show_addrinfo(struct addrinfo *ai)
{
	assert(ai != NULL);

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
	if (ai->ai_family == AF_INET)
		inet_ntop(ai->ai_family,
			  &((struct sockaddr_in *)(ai->ai_addr))->sin_addr,
			  ipstr, sizeof(ipstr));
	else if (ai->ai_family == AF_INET6)
		inet_ntop(ai->ai_family,
			  &((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr,
			  ipstr, sizeof(ipstr));
	else
		ipstr[0] = 0;
	printf("addr: %s\n", ipstr);
	printf("canoname: %s\n", ai->ai_canonname);
}

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *aip;
	int s;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((s = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	for (aip = res; aip != NULL; aip = aip->ai_next)
		show_addrinfo(aip);

	exit(EXIT_SUCCESS);
}
