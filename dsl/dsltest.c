//#include <iostream>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FILE_NAME "_dsltmp.c"
#define OBJ_NAME "_dsltmp.o"
#define SO_NAME "libdsltest.so.1"
#define OUT_NAME "libdsltest.so.1.0.1"

void create_file()
{
	const char *src = "//extern \"C\" {\n"
			  "	double myfunc(double d)\n"
			  "	{\n"
			  "		return d + (long)d;\n"
			  "	}\n"
			  "//}\n";
	int fd = open(FILE_NAME, O_CREAT|O_WRONLY|O_CLOEXEC, S_IWUSR|S_IRUSR);
	write(fd, src, strlen(src));
	close(fd);
}

void compile()
{
	pid_t pid = fork();
	if (pid != 0) {
		int status = 0;
		waitpid(pid, &status, 0);
		if (status != 0) {
			printf("cc error\n");
		}
	} else {
		execlp("gcc", "gcc", "-c", "-fPIC", FILE_NAME, NULL);
	}
	pid = fork();
	if (pid != 0) {
		int status = 0;
		waitpid(pid, &status, 0);
		if (status != 0) {
			printf("link error\n");
		}
	} else {
		const char *soname = "-Wl,-soname," SO_NAME;
		execlp("gcc", "gcc", "-shared", soname, "-o", OUT_NAME, OBJ_NAME, NULL);
	}
}

void *handle = NULL;
double myfunc(double);
double (*funcp)(double) = NULL;

void load()
{
	handle = dlopen(OUT_NAME, RTLD_LAZY);
	if (handle == NULL) {
		printf("dlopen error\n");
		return;
	}
	dlerror();
	funcp = (double (*)(double))dlsym(handle, "myfunc");
	if (funcp == NULL) {
		printf("dlsym error: %s\n", dlerror());
	}
}


int main(int argc, char *argv[])
{
	struct timespec ts0;
	clock_gettime(CLOCK_REALTIME, &ts0);

	create_file();
	compile();
	load();

	struct timespec ts1;
	clock_gettime(CLOCK_REALTIME, &ts1);

	ts1.tv_sec -= ts0.tv_sec;
	ts1.tv_nsec -= ts0.tv_nsec;
	if (ts1.tv_nsec < 0) {
		ts1.tv_sec--;
		ts1.tv_nsec += 1000000000;
	}

	printf("timecost: %ld.%03ld, funcp=%p\n", (long)ts1.tv_sec, (long)ts1.tv_nsec/1000000, funcp);

	double d = 1.18;
	printf("    %lf -> %lf\n", d, funcp(d)); 

	dlclose(handle);
}
