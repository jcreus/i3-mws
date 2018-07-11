#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <linux/limits.h>
#include <i3/ipc.h>
#include "json.h"

char *WORKSPACES[] = {"eDP1", "HDMI1"};

int max(int a, int b) {
	return a > b ? a : b;
}

void rename_initial_workspaces() {
	for (char c='0'; c<='9'; c++) {
		char buf[10];
		sprintf(buf, "%d:%c.%c", 10+(c-'0'), '1', c);
		char buf2[2] = {c, 0};
		char *args[] = {"i3", "rename","workspace",buf2,"to",buf, 0};
		if (fork() == 0) execv("/usr/bin/i3",args);
	}
}

/*
void move_workspaces(char base) {
	for (char c='0'; c<='9'; c++) {
		char buf[10];
		sprintf(buf, "%d:%c.%c", 10*(base-'0')+(c-'0'), base, c);
		char buf2[2] = {c, 0};
		char *args[] = {"i3", "rename","workspace",buf2,"to",buf, 0};
		if (fork() == 0) execv("/usr/bin/i3",args);
	}
}
*/

void get_i3_ipc_path(char *path, int sz) {
	FILE *fp = popen("i3 --get-socketpath", "r");
	if (fp == NULL) {
		printf("couldn't find i3 IPC socket");
		exit(1);
	}

	fgets(path, 255, fp);
	path[strlen(path)-1] = '\0';
	printf("Socket path: %s\n", path);
	pclose(fp);
}

int connect_i3_ipc() {
	char path[256];
	get_i3_ipc_path(path, 255);

        struct sockaddr_un i3_addr;
        int i3_socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if(i3_socket_fd < 0) {
                printf("socket() failed\n");
		exit(1);
        }

        memset(&i3_addr, 0, sizeof(struct sockaddr_un));

        i3_addr.sun_family = AF_UNIX;
        snprintf(i3_addr.sun_path, PATH_MAX, "%s", path);

        if(connect(i3_socket_fd, (struct sockaddr*) &i3_addr, sizeof(struct sockaddr_un)) != 0) {
                printf("connect() failed\n");
		exit(1);
        }
	char subscribe[] = "i3-ipc12341234[ \"workspace\", \"output\" ]\n";
	int len = strlen(subscribe+14);
	memcpy(subscribe+6, (char*)&len, 4);
	int type = I3_IPC_MESSAGE_TYPE_SUBSCRIBE;
	memcpy(subscribe+10, (char*)&type, 4);
	write(i3_socket_fd, subscribe, sizeof(subscribe));
	for (int i=0; i<sizeof(subscribe); i++) {
		printf("%x ", (int)subscribe[i]);
	}

	printf("subscribed to i3 ipc socket\n");
	return i3_socket_fd;
}

int connect_mws_ipc() {
	struct sockaddr_un addr;
	socklen_t addr_len;

	int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		printf("socket() failed\n");
		return 1;
	}

	unlink("./mws_socket");

	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, PATH_MAX, "./mws_socket");

	if(bind(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) != 0) {
		printf("bind() failed\n");
		return 1;
	}

	if(listen(socket_fd, 5) != 0) {
		printf("listen() failed\n");
		return 1;
	}

	return socket_fd;
}

int main(void) {

	int outputs[256] = {0};
	char master = '1';
	char last[256];
	memset(last, '1', 256);

	usleep(10000);
	rename_initial_workspaces();
	
        int i3_socket_fd = connect_i3_ipc();

	int socket_fd = connect_mws_ipc();

	fd_set set;

	/* Hardcoded times, courtesy of a hack I needed to get a quirky multimonitor setup working. */
	time_t last_screen_change = time(0)-5;
	time_t last_move = time(0)-5;

	while (1) {
		printf("waiting...\n");
		FD_ZERO(&set);
		FD_SET(socket_fd, &set);
		FD_SET(i3_socket_fd, &set);

		int rv = select(max(socket_fd, i3_socket_fd)+1, &set, NULL, NULL, NULL);
		int conn;

		if (FD_ISSET(socket_fd, &set)) {
			printf("hurrah!\n");
			struct sockaddr_un addr;
			memset(&addr, 0, sizeof(struct sockaddr_un));
			socklen_t addr_len = sizeof(addr);
			if ((conn = accept(socket_fd, (struct sockaddr*)&addr, &addr_len)) > -1) {
				int nbytes;
				char buffer[4] = "a.a";
				char data[4];
				nbytes = read(conn, data, 4);
				printf("got %c %c\n", data[0], data[2]);
				if (data[2] == 'm' || data[2] == 'w') {
					if (data[2] == 'w') {
						buffer[0] = data[0];
						buffer[2] = last[master];
					} else {
						buffer[0] = master;
						buffer[2] = data[0];
					}
					char string[10] = {0};
					char *args[] = {"i3","move","container",
							"to","workspace", string, 0};
					sprintf(string, "%d:%c.%c", 10*(buffer[0]-'0')+(buffer[2]-'0'),
									buffer[0], buffer[2]);
					if (fork() == 0) {
						close(socket_fd); close(i3_socket_fd); close(conn);
						execv("/usr/bin/i3",args);
					}
				} else if (data[2] == 's' || data[2] == 'k') {
					char string[10] = {0};
					if (data[2] == 's') {
						master = buffer[4];
					} else if (data[2] == 'k') {
						last[master] = data[0];
					}
					sprintf(string, "%d:", 10*(master-'0')+(last[master]-'0'));
					buffer[0] = master;
					buffer[2] = last[master];
					memcpy(string + strlen(string), buffer, 4);
					char *args[] = {"i3", "workspace", string, 0};
					if (fork() == 0) {
						close(socket_fd); close(i3_socket_fd); close(conn);
						execv("/usr/bin/i3",args);
					}
				}
			}
			close(conn);
		}
		if (FD_ISSET(i3_socket_fd, &set)) {
			char i3_buffer[32768];
			int num = read(i3_socket_fd, i3_buffer, 32768);
			//printf("hey! got stuff! %d %d\n", num, (int)(strstr(i3_buffer, "{")-i3_buffer));
			if (num > 25 && strncmp(i3_buffer+14, "{\"change\":\"focus\"", 16) == 0) {
				json_object *obj = json_tokener_parse(i3_buffer+14);
				json_object *current = json_object_object_get(obj, "current");
				json_object *name = json_object_object_get(current, "name");
				const char *namestr = json_object_get_string(name);
				printf("hmm %s", namestr);
				int ln = strlen(namestr);
				int num;
				if (ln == 1) num = namestr[0]-'0';
				else {
					num = 10*(namestr[0] - '0') + (namestr[1] - '0');
				}
				master = (num/10) + '0';
				last[master] = '0' + (num%10);
				printf("GOT %c %c\n", master, last[master]);
				json_object_put(obj);
			} else if (num > 14 && strncmp(i3_buffer+14, "{\"change\":\"unspecified\"", 20) == 0
						&& (time(0) - last_screen_change) > 5) {
				char workspaces[] = "i3-ipc12341234";
				int len = 0;
				memcpy(workspaces+6, (char*)&len, 4);
				int type = 1;
				memcpy(workspaces+10, (char*)&type, 4);
				write(i3_socket_fd, workspaces, sizeof(workspaces));
				last_screen_change = time(0);
			} else if (num > 50 && i3_buffer[14] == '[' && (time(0) - last_move) > 5) {
				json_object *obj = json_tokener_parse(i3_buffer+14);
				int len = json_object_array_length(obj);
				int maxidx = 0;
				int stop = 0;
				for (int i=0; i<len; i++) {
					json_object *val = json_object_array_get_idx(obj, i);
					if (val == 0) {
						stop = 1;
						break;
					}
					int num = json_object_get_int(json_object_object_get(val, "num"));
					const char *str = json_object_get_string(json_object_object_get(val, "output"));
					int idx = 0;
					for (; idx<2; idx++) {
						if (strcmp(str, WORKSPACES[idx]) == 0) break;
					}
					if (idx > maxidx) maxidx = idx;
					printf("have %d @ %s\n", num, str);
				}
				if (stop) continue;
				sleep(2);
				last_move = time(0);

				for (int i=0; i<len; i++) {
					json_object *val = json_object_array_get_idx(obj, i);
					int num = json_object_get_int(json_object_object_get(val, "num"));
					const char *str = json_object_get_string(json_object_object_get(val, "output"));
					int idx = 0;
					for (; idx<2; idx++) {
						if (strcmp(str, WORKSPACES[idx]) == 0) break;
					}
					if (maxidx == 0) {
						printf("Restoring %d to %d\n", num, outputs[num]);
						char string[10] = {0};
						sprintf(string, "%d:%c.%c", num, (num/10)+'0',
										(num%10)+'0');
						char *args[] = {"i3", "workspace", string, 0};
						if (fork() == 0) {
							close(socket_fd);
							close(i3_socket_fd);
							close(conn);
							execv("/usr/bin/i3",args);
						}
						usleep(1000*400);
						char *args2[] = {"i3", "move","workspace", "to",
									"output",
									WORKSPACES[outputs[num]], 0};
						if (fork() == 0) {
							close(socket_fd);
							close(i3_socket_fd);
							close(conn);
							execv("/usr/bin/i3",args2);
						}

					} else if (maxidx > 0) {
						printf("Remembering %d is in %d\n", num, idx);
						outputs[num] = idx;
					}
				}
				json_object_put(obj);
			} else {
				//for (int c=0; c<num; c++) printf("%c", i3_buffer[c]);
				//printf("\n");
			}
		}
	}

	close(socket_fd);
	unlink("./mws_socket");
	return 0;
}
