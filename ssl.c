// gcc ssl.c -lssl  -lcrypto

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include <string.h>

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define IP_ADDR INADDR_ANY
#define PORT 1001

int password_cb(char *buf, int size, int rwflag, void *password);

int ssl_run(int cfd) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
  if (ctx == NULL) {
    printf("errored; unable to load context.\n");
    ERR_print_errors_fp(stderr);
    return -3;
  }

  struct timespec tp[3];
  clock_gettime(CLOCK_REALTIME, &tp[0]);
  SSL_CTX_use_certificate_file(ctx, "gw.pem", SSL_FILETYPE_PEM);
  SSL_CTX_set_default_passwd_cb(ctx, password_cb);
  SSL_CTX_use_PrivateKey_file(ctx, "gw.key", SSL_FILETYPE_PEM);
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

  SSL *ssl = SSL_new(ctx);

  BIO *accept_bio = BIO_new_socket(cfd, BIO_CLOSE);
  SSL_set_bio(ssl, accept_bio, accept_bio);

  SSL_accept(ssl);
  ERR_print_errors_fp(stderr);
  BIO *bio = BIO_pop(accept_bio);

  clock_gettime(CLOCK_REALTIME, &tp[1]);
  char buf[8192];
  while (1) {
    // first read data
    int r = SSL_read(ssl, buf, sizeof buf); 
    switch (SSL_get_error(ssl, r)) { 
      case SSL_ERROR_NONE: 
        break;
      case SSL_ERROR_ZERO_RETURN: 
        goto end; 
      default: 
        printf("SSL read problem");
        goto end;
    }

      
    clock_gettime(CLOCK_REALTIME, &tp[2]);
    long sh_time = (tp[1].tv_sec - tp[0].tv_sec) * 1000 + (tp[1].tv_nsec - tp[0].tv_nsec) / 1000000;
    long serv_time = (tp[2].tv_sec - tp[1].tv_sec) * 1000000 + (tp[2].tv_nsec - tp[1].tv_nsec) / 1000;
    char content[200];
    int cnt_len = snprintf(content, sizeof (content), "ssl hank shake: %ldms, service: %ldus\r\n", sh_time, serv_time);
    int len = snprintf(buf, sizeof buf,
                                    "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/plain\r\n"
                                    "Content-Length: %d\r\n"
                                    "Access-Control-Allow-Origin: *\r\n\r\n%s", cnt_len, content);

    // now keep writing until we've written everything
    int offset = 0;
    while (len) {
      r = SSL_write(ssl, buf + offset, len); 
      switch (SSL_get_error(ssl, r)) { 
        case SSL_ERROR_NONE: 
          len -= r;
          offset += r; 
          break;
        default:
          printf("SSL write problem");
          goto end;
      }
    }
    break;
  }
end:
  SSL_shutdown(ssl);

  BIO_free_all(bio);
  BIO_free_all(accept_bio);
}

int create_server() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("opening socket\n");
    return -4;
  }

  struct sockaddr_in s_addr;
  bzero((char *)&s_addr, sizeof(s_addr));
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = IP_ADDR;
  s_addr.sin_port = htons(PORT);

  if (bind(fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
    perror("binding\n");
    return -5;
  }

  SSL_load_error_strings();
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  SSL_library_init();

  listen(fd, 2);
  return fd;
}

/**
 * Example SSL server that accepts a client and echos back anything it receives.
 * Test using `curl -I https://127.0.0.1:8081 --insecure`
 */
int main(int arc, char **argv) {
  int fd = create_server(); 
  if (fd < 0) {
    exit(1);
  }

  int cfd;
  while ((cfd = accept(fd, 0, 0)) != -1) {
    ssl_run(cfd);
    close(cfd);
  }

  return 0;
}

int password_cb(char *buf, int size, int rwflag, void *password) {
    strncpy(buf, (char *)(password), size);
    buf[size - 1] = '\0';
    return strlen(buf);
}
