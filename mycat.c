#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int myopen(const char *pathname, int flags)
{
	pid_t pid;

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "fork: %m\n");	
		exit(1);
	} else if (pid == 0) {
	
	}

	int status;
	waitpid(pid, &status, 0);

}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "usage: mycat <pathname>\n");	
		exit(1);
	}

	int fd;
	fd = myopen(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "can't open file %s\n", argv[1]);	
		exit(1);
	}
#define BUFSIZ	512
	ssize_t n;
	char buf[BUFSIZ];
	while ((n = read(fd, buf, BUFSIZ)) > 0)
		write(STDOUT_FILENO, buf, n);

	exit(0);
}
