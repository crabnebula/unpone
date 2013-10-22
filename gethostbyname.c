#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define handle_error(msg, err) ({					\
		fprintf(stderr, "%s: %s\n", msg, h_strerror(err));	\
		exit(EXIT_FAILURE);					\
	 })

static void show_hostent(struct hostent *host)
{
	char **cpp;
	char ipstr[INET6_ADDRSTRLEN];

	assert(host);
	printf("host name: %s\n", host->h_name);
	printf("host aliases: ");
	for (cpp = host->h_aliases; *cpp; cpp++)
		printf("%s ", *cpp);
	printf("\n");
	printf("host addrtype: %s\n",
		host->h_addrtype == AF_INET ? "ipv4" :
 		host->h_addrtype == AF_INET6 ? "ipv6" : "unknown");
	printf("host length: %d\n", host->h_length);
	if (host->h_addrtype != AF_INET && 
	    host->h_addrtype != AF_INET6)
		return;
	printf("host addr list: ", *host->h_addr_list);
	for (cpp = host->h_addr_list; *cpp; cpp++)
		printf("%s ", inet_ntop(host->h_addrtype, *cpp,
					ipstr, sizeof(ipstr)));
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct in_addr addr4;
	struct in6_addr addr6;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <host>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	struct hostent *host = NULL;

	host = gethostbyname(argv[1]);
	if (host == NULL) {
		printf("%s is not hostname\n", argv[1]);
		if (inet_pton(AF_INET, argv[1], &addr4) == 0)
			host = gethostbyaddr(&addr4, sizeof(addr4), AF_INET);
		else if (inet_pton(AF_INET6, argv[1], &addr6) == 0)
			host = gethostbyaddr(&addr6, sizeof(addr6), AF_INET6);
	}
	if (host == NULL) {
		fprintf(stderr, "unknown host %s\n", argv[1]);	
		exit(EXIT_FAILURE);
	}
	show_hostent(host);

	exit(EXIT_SUCCESS);
}
