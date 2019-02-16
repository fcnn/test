// gcc http.c -Wall -lrt

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
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

#define HTML "<!DOCTYPE html>" \
"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"zh-CN\" xml:lang=\"zh-CN\">" \
"	<head> <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"> <title>Network status</title> </head>" \
"<body>" \
"		<p id=\"body_title\"></p>" \
"		<div id=\"msg\"></div>" \
"       <textarea id=\"rsp\" style=\"width: 100%; height: 50px\" readonly=\"readonly\"></textarea><br />" \
"<script>" \
"var msg = document.getElementById(\"msg\");" \
"var rsp = document.getElementById(\"rsp\");" \
"var startTime;" \
"function getCode(url, id) {" \
"    fetch(url + \"ping\", {" \
"        method: \"POST\"," \
"        headers: {" \
"            \"Content-Type\": \"text/plain\"," \
"        }," \
"        body: id + \"\\r\\n\\r\\n\"" \
"    })" \
"    .then(res => res.json())" \
"    .then(data => {" \
"		setTimeIndicator(startTime);" \
"       rsp.value = rsp.value + ' ' + id + \"=>\" + JSON.stringify(data);" \
"    }).catch(function(error) {" \
"	    rsp.value = rsp.value + ' ' + id + '=>error: ' + error.message;" \
"    });" \
"}" \
"function setTimeIndicator(start) {" \
"    let now = new Date();" \
"	document.getElementById('body_title').innerHTML = now.toString() + '！';" \
"	msg.innerHTML = 'round trip time: ' + (now.getTime() - start).toString() + 'ms';" \
"}" \
"rsp.value = '';" \
"startTime = new Date().getTime();" \
"getCode(\"http://h2.yeejay.cc:800/\", \"h2\");" \
"getCode(\"http://f1.yeejay.cc:800/\", \"f1\");" \
"</script> </body> </html>"


#define IP_ADDR INADDR_ANY
#define PORT 800

static void print_peer_info(struct sockaddr *sa, socklen_t slen) {
	char name[128];
	char service[64];
	getnameinfo(sa, slen,
			name, sizeof(name), service, sizeof(service),
			NI_NUMERICHOST | NI_NUMERICSERV);
	fprintf(stderr, "accepted %s/%s.\n", name, service);
}

static void http_run(int sd) {
  struct timespec tp[2];
  clock_gettime(CLOCK_REALTIME, &tp[0]);

  char buf[4096];
  while (1) {
    // first read data
    int offset = 0;
	int request = -1;
    while (offset < sizeof(buf)-1) {
      int r = read(sd, buf + offset, sizeof (buf) - 1 - offset);
      if (r == -1 && errno == EINTR) {
        continue;
      }

      if (r < 1) {
        printf("%s\n", r==0?"connection lost":"read error");
        goto end;
      }
      offset += r;
      if (offset>=4 && buf[offset-4]=='\r' && buf[offset-3]=='\n' && buf[offset-2]=='\r' && buf[offset-1]=='\n') {
        request = 0;
        buf[offset] = 0;
        if (strstr(buf, "POST") != NULL && strstr(buf, "/ping") != NULL) {
          request = 1;
        }
        fprintf(stderr, "-> %s\n", buf);
        break;
      } else {
        buf[offset] = 0;
        fprintf(stderr, "--> [%s]\n", buf);
      }
    }

    if (request < 0) {
      goto end;
    }
    char *body;
    int cnt_len;
    char data[2048];
    const char *content_type = "text/html";
    if (request == 0) {
      body = HTML;
      cnt_len = strlen(body);
    } else {
      body = data;
      clock_gettime(CLOCK_REALTIME, &tp[1]);
      double serv_time = (double)((tp[1].tv_sec - tp[0].tv_sec) * 1000000000 + (tp[1].tv_nsec - tp[0].tv_nsec)) / 1000000;
      cnt_len = snprintf(data, sizeof (data), "{\"elapsed\": %.3f}", serv_time);
      content_type = "application/json";
    }

    int len = snprintf(buf, sizeof buf,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"Access-Control-Allow-Origin: *\r\n\r\n%s\r\n", content_type, cnt_len, body);

    // now keep writing until we've written everything
    offset = 0;
    while (len) {
      int r = write(sd, buf + offset, len); 
      if (r > 0) {
        len -= r;
        offset += r; 
      } else {
        if (r == -1 && errno == EINTR) {
          continue;
        }
		printf("write problem");
        goto end;
      }
    }
    break;
  }
end:
  close(sd);
}

int create_server(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("opening socket\n");
    return -4;
  }

  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,  &yes, sizeof(yes)) != 0) {
    printf("error set SO_REUSEADDR\n");
    return -5;
  }

  struct sockaddr_in s_addr;
  bzero((char *)&s_addr, sizeof(s_addr));
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = IP_ADDR;
  s_addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
    perror("binding\n");
    return -6;
  }

  listen(fd, 2);
  return fd;
}

int main(int argc, char **argv) {
  int fd = create_server(argc>1?atoi(argv[1]):PORT); 
  if (fd < 0) {
    exit(1);
  }

  int cd;
  socklen_t slen;
  struct sockaddr_in6 sa;

  while ((cd = accept(fd, (struct sockaddr*)&sa, &slen)) != -1) {
    print_peer_info((struct sockaddr*)&sa, slen);
    http_run(cd);
    close(cd);
  }

  return 0;
}
