#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>

ssize_t readfd(int sockfd, void *ptr, size_t len, int *recvfd)
{
	struct msghdr msg;
	struct cmsghdr *cmsg;
	ssize_t n;
	struct iovec iov[1];

	union {
		struct cmsghdr cmsg;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = ptr;
	iov[0].iov_len = len;
	msg.msg_iov= iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	
	if ((n = recvmsg(sockfd, &msg, 0)) == -1) {
		fprintf(stderr, "recvmsg: %m\n");
		exit(1);
	}
	for (cmsg = CMSG_FIRSTHDR(&msg);
	     cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET &&
		    cmsg->cmsg_type == SCM_RIGHTS) {
		    *recvfd = *(int *)CMSG_DATA(cmsg);
		    break;
		}
	}
	if (cmsg == NULL) {
		fprintf(stderr, "not receive fd\n");
		exit(1);
	}

	return n;
}

int myopen(const char *pathname, int flags)
{
	pid_t pid;
	int sockfd[2];
	int recvfd;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == -1) {
		fprintf(stderr, "sockpair: %m\n");
		exit(1);
	}
	printf("sock1 = %d sock2 = %d\n", sockfd[0], sockfd[1]);
	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "fork: %m\n");	
		exit(1);
	} else if (pid == 0) {
		close(sockfd[0]);
		char fdstr[32];
		char flagstr[32];	
		snprintf(fdstr, sizeof(fdstr), "%d", sockfd[1]);
		snprintf(flagstr, sizeof(flagstr), "%d", flags);
		execl("./openfile", "openfile", fdstr, pathname, flagstr, NULL);
		exit(1);
	}

	close(sockfd[1]);
	int status;
	char c;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status))
		exit(1);
	if (WEXITSTATUS(status) == 0) {
		if (readfd(sockfd[0], &c, 1, &recvfd) < 0)
			exit(1);
	}

	return recvfd;
}

#undef BUFSIZ
#define BUFSIZ	512
int main(int argc, char *argv[])
{
	int fd;
	ssize_t n;
	char buf[BUFSIZ];

	if (argc < 2) {
		fprintf(stderr, "usage: mycat <pathname>\n");	
		exit(1);
	}

	fd = myopen(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "can't open file %s\n", argv[1]);	
		exit(1);
	}
	printf("pid %d recvfd = %d\n", getpid(), fd);
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		write(STDOUT_FILENO, buf, n);

	exit(0);
}
