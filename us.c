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

#define SERVICE_NAME "5099"

static int
init_server(int argc, char *argv[])
{
	int sd;
	int res;
	struct addrinfo hints;
	struct addrinfo *addr_i;
	struct addrinfo *hostaddr = 0;

	char *host = 0;
	char *service = SERVICE_NAME;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

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
		hints.ai_flags = AI_CANONNAME | AI_NUMERICSERV;
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

		printf("success\n");
		break;
	}

	freeaddrinfo(hostaddr);

	return addr_i == 0 ? -1 : sd; 
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
	return 0;
}

int
main(int argc, char *argv[])
{
	int sd;
	ssize_t len;
	socklen_t slen;
	char buf[1024];
	struct sockaddr_in6 sa;

	if (init_sig(argc, argv) != 0) {
		exit(1);
	}

	sd = init_server(argc, argv);
	if (sd == -1) {
		exit(1);
	}

	for ( ; ; ) {
		//printf("-> ");
		//fflush(stdout);
		slen = sizeof (sa);
		len = recvfrom(sd, buf, sizeof (buf), 0, (struct sockaddr *)&sa, &slen);
		if (len != -1) {
#if 0
			char name[128];
			char serv_name[64];
			getnameinfo((struct sockaddr *)&sa, slen,
				name, sizeof(name), serv_name, sizeof(serv_name),
				NI_NUMERICHOST | NI_NUMERICSERV);
			printf("recved %ld bytes from %s/%s.\n", (long)len, name, serv_name);
#endif //0
			sendto(sd, buf, len, 0, (struct sockaddr *)&sa, slen);
		}
		else {
			if (errno == EINTR) {
				continue;
			}
			perror("recvfrom");
			break;
		}
	}

	close(sd);

	return 0;
}

