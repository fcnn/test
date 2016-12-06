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
#include <fcntl.h>
#include <time.h>

void http_run(int sd)
{
	ssize_t n;
	ssize_t len=1;
	char msg[1024];

	for ( ; len > 0 || ( len < 0 && errno == EINTR); ) {
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
        	} else {
			close(sd);
			msg[len] = 0;
			printf("[%s]\n", msg);
			return;
/*
               		int sent_len = send(sd, msg, len, 0);
			if (sent_len == -1) {
                       		 perror("send");
				if (errno == EPIPE || errno == ENOTCONN) {
					printf("connection closed during send ...\n");
				}
				return;
			}
			printf("sent %d\n", sent_len);
*/
        	}
	}
}

