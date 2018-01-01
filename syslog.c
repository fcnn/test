#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>

int main(int argc, char *argv) {
        openlog("miner-hacker", 0, LOG_USER);
        time_t t = time(NULL);
        syslog(LOG_ALERT, "pid = %ld, ppid = %ld, uid = %ld, euid = %ld",
                        (long)getpid(), (long)getppid(), (long)getuid(), (long)geteuid());
        closelog();
        //for ( ; ; ) {
        //      sleep(10);
        //}
}

