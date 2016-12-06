#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <time.h>

#include "http.h"

#define SERVICE_NAME "http"

struct __epoll_server {
	int epollfd;
	int listen_sd[8];
	int sd_count;
};

static int set_epoll(struct __epoll_server *server)
{
	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		return -1;
	}
	for (int i = 0; i < server->sd_count; ++i) {
		int sd = server->listen_sd[i];
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = sd;
		int flags = fcntl(sd, F_GETFL, 0);
		if (fcntl(sd, F_SETFL, flags | O_NONBLOCK) == -1) {
			perror("fcntl");
			return -1;
		}
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sd, &ev) == -1) {
			perror("epoll_ctl");
			close(epollfd);
			return -1;
		}
	}
	server->epollfd = epollfd;
	return epollfd;
}
 
static int init_server(int argc, char *argv[], struct __epoll_server *epoll_server)
{
	int sd;
	int res;
	struct addrinfo hints;
	struct addrinfo *addr_i;
	struct addrinfo *hostaddr = 0;

	char *host = 0;
	const char *service = SERVICE_NAME;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST | /*AI_NUMERICSERV |*/ AI_PASSIVE;

	if (argc > 1) {
		if (strcmp(argv[1], "-") != 0) {
			host = argv[1];
		}
		if (argc > 2) {
			service = argv[2];
		}
	}

	printf("resolving %s/%s ... ", host == 0 ? "*" : host, service);
	fflush(stdout);
	res = getaddrinfo(host, service, &hints, &hostaddr);
	if (res == EAI_NONAME) {
		hints.ai_flags = AI_CANONNAME | AI_NUMERICSERV | AI_PASSIVE;
		res = getaddrinfo(host, service, &hints, &hostaddr);
	}

	if (res != 0 || hostaddr == 0) {
		printf("failed\n");
		return -1;
	}

	printf("success\n");

	const int max_sd = sizeof epoll_server->listen_sd / sizeof epoll_server->listen_sd[0];

	epoll_server->sd_count = 0;
	for (addr_i = hostaddr; addr_i != 0; addr_i = addr_i->ai_next) {
		char name[128];
		char serv_name[64];
		getnameinfo(addr_i->ai_addr, addr_i->ai_addrlen,
			name, sizeof(name), serv_name, sizeof(serv_name),
			NI_NUMERICHOST | NI_NUMERICSERV);
		printf("binding %s/%s ... ", name, serv_name);
		fflush(stdout);
		if ((sd = socket(addr_i->ai_family, addr_i->ai_socktype, 0)) == -1) {
			perror("socket");
			continue;
		}

		if (bind(sd, (struct sockaddr *)addr_i->ai_addr, addr_i->ai_addrlen) == -1) {
			perror("bind");
			close(sd);
			continue;
		}

		if (listen(sd, 5) == 0) {
			printf("success\n");
			epoll_server->listen_sd[epoll_server->sd_count++] = sd;
			if (epoll_server->sd_count == max_sd) {
				break;
			}
		} else {
			perror("listen");
			close(sd);
		}
	}

	freeaddrinfo(hostaddr);

	if (epoll_server->sd_count == 0)
		return -1;

	int epfd = set_epoll(epoll_server); 
	if (epfd == -1) {
		for (int i = 0; i < epoll_server->sd_count; ++i) {
			close(epoll_server->listen_sd[i]);
		}
		return -1;
	}

	return 0;
}

static void sig_child(int signo, siginfo_t *siginfo, void *foo)
{
	int status;
	struct rusage rusage;
	for ( ; wait4(-1, &status, WNOHANG, &rusage) > 0; ) {
	}
}

static int init_sig(int argc, char *argv[])
{
	struct sigaction act, oact;

	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &sig_child;
	if (sigaction(SIGCHLD, &act, &oact) != 0) {
		return -1;
	}
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, &oact) != 0) {
		return -1;
	}
	return 0;
}

void peer_info(int cd, struct sockaddr *sa, socklen_t slen)
{
	char name[128];
	char serv_name[64];
	getnameinfo(sa, slen, name, sizeof(name), serv_name, sizeof(serv_name), 0);
	//NI_NUMERICHOST | NI_NUMERICSERV);
	int flags = fcntl(cd, F_GETFL, 0);
	if (fcntl(cd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl");
	}
	flags = fcntl(cd, F_GETFL, 0);
	printf("accepted %s/%s. noneblock=%d\n", name, serv_name, flags & O_NONBLOCK);
}

int main(int argc, char *argv[])
{
	if (init_sig(argc, argv) != 0) {
		exit(1);
	}

	struct __epoll_server epoll_server;
	if (init_server(argc, argv, &epoll_server) == -1) {
		exit(1);
	}

	const int max_events = 10;
	struct epoll_event events[max_events];
	for ( ; ; ) {
		printf("-> ");
		fflush(stdout);
		int nfds = epoll_pwait(epoll_server.epollfd, events, max_events, 15000, NULL);
		if (nfds == -1) {
			perror("epoll_pwait");
			break;
		}
		for (int i = 0; i < nfds; ++i) {
			int j = 0;
			for ( ; j < epoll_server.sd_count; ++j ) {
				if (events[i].data.fd == epoll_server.listen_sd[j])
					break;
			}
			if (j >= epoll_server.sd_count) {
				http_run(events[i].data.fd);
				continue;
			}
			struct sockaddr_in6 sa;
			socklen_t slen = sizeof (sa);
			int cd = accept(epoll_server.listen_sd[j], (struct sockaddr *)&sa, &slen);
			if (cd != -1) {
				peer_info(cd, (struct sockaddr *)&sa, slen);
				struct epoll_event ev;
				ev.events = EPOLLIN|EPOLLET;
				ev.data.fd = cd;
				if (epoll_ctl(epoll_server.epollfd, EPOLL_CTL_ADD, cd, &ev) == -1) {
					perror("epoll_ctl");
					close(cd);
				}
			} else {
				if (errno == EINTR) {
					continue;
				}
				perror("accept");
				break;
			}
		}
	}

	for (int i = 0; i < epoll_server.sd_count; ++i)
		close(epoll_server.listen_sd[i]);
	close(epoll_server.epollfd);

	return 0;
}

