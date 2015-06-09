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

static void
service_main(int sd, struct sockaddr *sa, int salen)
{
	ssize_t n;
	ssize_t len;
	char msg[1024];

	for ( ; ; ) {
		len = recv(sd, msg, sizeof (msg), 0);
                if (len == 0) {
                        printf("connection closed by peer\n");
                        break;
                }
                if (len < 0) {
                        if (errno != EINTR) {
                                perror("recv");
                                break;
                        }
                }
                else {
                        len = send(sd, msg, len, 0);
                }
	}

}

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

void on_new_connection(int cd)
{
	int buflen;
	socklen_t optlen = sizeof buflen;
	int ret = getsockopt(cd,SOL_SOCKET,SO_RCVBUF,&buflen,&optlen);
	printf("rcv buff = %d\n", buflen);
	buflen = 32;
	ret = setsockopt(cd,SOL_SOCKET,SO_RCVBUF,&buflen,sizeof buflen);
	ret = getsockopt(cd,SOL_SOCKET,SO_RCVBUF,&buflen,&optlen);
	printf("rcv buff = %d\n", buflen);
}

int
main(int argc, char *argv[])
{
	int sd;
	int cd;
	socklen_t slen;
	struct sockaddr_in6 sa;

	if (init_sig(argc, argv) != 0) {
		exit(1);
	}

	sd = init_server(argc, argv);
	if (sd == -1) {
		exit(1);
	}

	for ( ; ; ) {
		printf("-> ");
		fflush(stdout);
		slen = sizeof (sa);
		cd = accept(sd, (struct sockaddr *)&sa, &slen);
		if (cd != -1) {
			char name[128];
			char serv_name[64];
			getnameinfo((struct sockaddr *)&sa, slen,
				name, sizeof(name), serv_name, sizeof(serv_name),
				NI_NUMERICHOST | NI_NUMERICSERV);
			printf("accepted %s/%s.\n", name, serv_name);
		}
		else {
			if (errno == EINTR) {
				continue;
			}
			perror("accept");
			break;
		}

		pid_t pid = fork();

		if (pid != 0) {
			close(cd);
		}
		else {
			close(sd);
			on_new_connection(cd);
			service_main(cd, (struct sockaddr*)&sa, slen);
			close(cd);
			exit(0);
		}
	}

	close(sd);

	return 0;
}


