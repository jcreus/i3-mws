#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>

int main(int argc, char **argv) {
	struct sockaddr_un addr;
	int socket_fd, nbytes;
	char buffer[4] = {0};

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		printf("socket() failed\n");
		return 1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, PATH_MAX, "./mws_socket");

	if(connect(socket_fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) != 0) {
		printf("connect() failed\n");
		return 1;
	}

	buffer[0] = argv[1][0];
	buffer[2] = argv[2][0];
	write(socket_fd, buffer, 4);

	close(socket_fd);

	return 0;
}
