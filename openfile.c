#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

ssize_t writefd(int sockfd, void *ptr, size_t len, int sendfd)
{
	struct msghdr msg;
	struct cmsghdr *cmsg;
	union {
		struct cmsghdr cmsg;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*(int *)CMSG_DATA(cmsg) = sendfd;

	struct iovec iov[1];
	iov[0].iov_base = ptr;
	iov[0].iov_len = len;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_flags = 0;

	ssize_t n;
	n = sendmsg(sockfd, &msg, 0);
	if (n < 0) {
		fprintf(stderr, "sendmsg: %m\n");		
		exit(1);
	}
	return n;
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		fprintf(stderr, "usage: openfile sockfd pathname flags\n");	
		exit(1);
	}
	int fd;
	fd = open(argv[2], atoi(argv[3]));
	if (fd == -1) {
		fprintf(stderr, "open: %m\n");
		exit(1);
	}
	printf("pid %d fd = %d\n", getpid(), fd);
	if (writefd(atoi(argv[1]), "", 1, fd) < 0) {
		fprintf(stderr, "writefd error\n");
		exit(1);
	}
	close(fd);
	close(atoi(argv[1]));
	exit(0);
}
