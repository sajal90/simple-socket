#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void error(char *s)
{
	fprintf(stderr, "%s: %s\n", s, strerror(errno));
	exit(EXIT_FAILURE);
}

int open_listener_socket() {
	int l = socket(PF_INET, SOCK_STREAM, 0);
	if (l == -1) {
		error("Couldn't open the listener socket");
	}

	return l;
}

int bind_to_port(int socket, int port)
{
	struct sockaddr_in name;
	name.sin_family = PF_INET;
	name.sin_port = (in_port_t)htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	int reuse = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int))) {
		error("Couldn't set port to reuse");
	}

	int b = bind(socket, (struct sockaddr *) &name, sizeof(name));
	if (b == -1) {
		error("Couldn't bind to port");
	}

	return b;
}

int say(int socket, char *s)
{
	int res = send(socket, s, strlen(s), 0);
	if (res == -1) {
		fprintf(stderr, "Couldn't send data: %s\n", strerror(errno));
	}

	return res;
}

int read_in(int socket, char *s, int len)
{
	char *t = s;
	int tlen = len;
	int c = recv(socket, t, tlen, 0);
	while (c > 0 && t[c-1] != '\n') {
		t += c;
		tlen -= c;
		c = recv(socket, t, tlen, 0);
	}

	if (c < 0)
		return c;
	else if (c == 0)
		s[0] = '\0';
	else
		s[c-1] = '\0';

	return len - tlen;
}

int listener_d;
void handle_shutdown(int sig)
{
	if(listener_d)
		close(listener_d);

	fprintf(stderr, "Bye!\n");
	exit(EXIT_SUCCESS);
}

int main()
{
	signal(SIGINT, handle_shutdown);
	
	listener_d = open_listener_socket();
	
	bind_to_port(listener_d, 30000);

	if (listen(listener_d, 10)) {
		error("Couldn't listen on port");
	}
	fprintf(stdout, "Waiting for connection\n");

	char arr[1024];
	while(1) {
		struct sockaddr_storage client;
		unsigned int client_size = sizeof(client);
		int connect_d =  accept(listener_d, (struct sockaddr *) &client, (socklen_t *) &client_size);
		if (connect_d == -1) {
			error("Can't open secondary socket");
		}
		
		if (!fork()) {
			// child process
			say(connect_d, "Knock! Knock!\n");
			read_in(connect_d, arr, sizeof(arr));

			if (strncasecmp("Who's there?\r\n", arr, 12) == 0) {
				say(connect_d, "Oscar\n");
				read_in(connect_d, arr, sizeof(arr));

				if (strncasecmp(arr, "Oscar who?", 10) == 0) {
					say(connect_d, "Oscar silly question, you get a silly answer\n");
				} else {
					say(connect_d, "You should say Oscar who?");
				}

			} else {
				say(connect_d, "You should say Who's there\n");
			}
			close(connect_d);
			close(listener_d);
			exit(EXIT_SUCCESS);
		}
		close(connect_d);
	}

	exit(EXIT_SUCCESS);
}
