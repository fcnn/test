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

#define SERVICE_NAME "5099"

static void
service_main(int sd)
{
	ssize_t n;
	char msg[16];
	ssize_t len=1;

	for ( ; len > 0 || ( len < 0 && errno == EAGAIN); ) {
	len = recv(sd, msg, sizeof (msg), 0);
        if (len == 0) {
		printf("connection closed by peer\n");
		close(sd);
		return;
        }
        if (len < 0) {
               if (errno != EINTR) {
			if (errno != EAGAIN) {
                        	perror("recv");
				close(sd);
			}
			return;
                }
        }
        else {
                int sent_len = send(sd, msg, len, 0);
		if (sent_len == -1) {
                        perror("send");
			if (errno == EPIPE || errno == ENOTCONN) {
				printf("connection closed during send ...\n");
			}
			return;
		}
		printf("sent %d\n", sent_len);
        }
	}
}

static int
init_epoll(int sd)
{
	int flags = fcntl(sd, F_GETFL, 0);
	if (fcntl(sd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl");
		return -1;
	}
	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		return -1;
	}
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sd, &ev) == -1) {
		perror("epoll_ctl");
		close(epollfd);
		return -1;
	}
	return epollfd;
}
 
struct __epoll_server {
	int epollfd;
	int listen_sd;
};

static int
init_server(int argc, char *argv[], struct __epoll_server *epoll_server)
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
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

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

	for (addr_i = hostaddr; addr_i != 0; addr_i = addr_i->ai_next) {
		char name[128];
		char serv_name[64];
		getnameinfo(addr_i->ai_addr, addr_i->ai_addrlen,
			name, sizeof(name), serv_name, sizeof(serv_name),
			NI_NUMERICHOST | NI_NUMERICSERV);
		printf("binding %s/%s ... ", name, serv_name);
		if ((sd = socket(addr_i->ai_family, addr_i->ai_socktype, 0)) == -1) {
			perror("");
			continue;
		}

		if (bind(sd, (struct sockaddr *)addr_i->ai_addr, addr_i->ai_addrlen) == -1) {
			perror("");
			close(sd);
			continue;
		}

		if (listen(sd, 5) == 0) {
			printf("success\n");
			break;
		}

		perror("listen");
		close(sd);
	}

	freeaddrinfo(hostaddr);

	if (addr_i == 0)
		return -1;

	int epfd = init_epoll(sd); 
	if (epfd == -1) {
		close(sd);
		return -1;
	}

	epoll_server->listen_sd = sd;
	epoll_server->epollfd = epfd;

	return 0;
}

static void
sig_child(int signo, siginfo_t *siginfo, void *foo)
{
	int status;
	struct rusage rusage;
	for ( ; wait4(-1, &status, WNOHANG, &rusage) > 0; ) {
	}
}

static int
init_sig(int argc, char *argv[])
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
	getnameinfo(sa, slen,
		name, sizeof(name), serv_name, sizeof(serv_name),
		0);
	//NI_NUMERICHOST | NI_NUMERICSERV);
	int flags = fcntl(cd, F_GETFL, 0);
	if (fcntl(cd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl");
	}
	flags = fcntl(cd, F_GETFL, 0);
	printf("accepted %s/%s. noneblock=%d\n", name, serv_name, flags & O_NONBLOCK);
}

int
main(int argc, char *argv[])
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
		int nfds = epoll_pwait(epoll_server.epollfd, events, max_events, 2000, NULL);
		if (nfds == -1) {
			perror("epoll_pwait");
			break;
		}
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == epoll_server.listen_sd) {
				struct sockaddr_in6 sa;
				socklen_t slen = sizeof (sa);
				int cd = accept(epoll_server.listen_sd, (struct sockaddr *)&sa, &slen);
				if (cd != -1) {
					peer_info(cd, (struct sockaddr *)&sa, slen);
					struct epoll_event ev;
					ev.events = EPOLLIN|EPOLLET;
					ev.data.fd = cd;
					if (epoll_ctl(epoll_server.epollfd, EPOLL_CTL_ADD, cd, &ev) == -1) {
						perror("epoll_ctl");
						close(cd);
					}
				}
				else {
					if (errno == EINTR) {
						continue;
					}
					perror("accept");
					break;
				}
			}
			else {
				//struct timespec req, rem;
				//req.tv_sec = 2;
				//req.tv_nsec = 100000000;
				//nanosleep(&req, &rem);
				service_main(events[i].data.fd);
			}
		}
	}

	close(epoll_server.listen_sd);
	close(epoll_server.epollfd);

	return 0;
}

