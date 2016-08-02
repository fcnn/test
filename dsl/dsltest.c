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
#include <stdlib.h>

#define FILE_NAME "_dsltmp.c"
#define OBJ_NAME "_dsltmp.o"
#define SO_NAME "libdsltest.so"

void create_file()
{
	const char *src = "//extern \"C\" {\n"
			  "     #include <stdio.h>\n"
			  "	void _init()\n"
			  "	{\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "	void _fini()\n"
			  "	{\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "     __attribute__((constructor)) void constructor()\n"
			  "     {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "     }\n"
			  "     __attribute__((destructor)) void destructor()\n"
			  "     {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "     }\n"
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
		execlp("gcc", "gcc", "-shared", "-nostartfiles", soname, "-o", SO_NAME, OBJ_NAME, NULL);
	}
}

void *handle = NULL;
double myfunc(double);
double (*funcp)(double) = NULL;

void set_ld_path()
{
	char ld_path[1024];
	const char *name = "LD_LIBRARY_PATH";
	char *oldpath = getenv(name);
	if (oldpath == NULL) {
		getcwd(ld_path, sizeof ld_path);
	} else {
		int len = sprintf(ld_path, "%s:", oldpath);
		getcwd(ld_path + len, sizeof (ld_path) - len);
	}
	printf("setting %s to %s ...\n", name, ld_path);
	setenv(name, ld_path, 1);
	if (fork() != 0) exit(0);
}
void load()
{
	//set_ld_path();
	handle = dlopen(SO_NAME, RTLD_LAZY);
	if (handle == NULL) {
		printf("dlopen error: %s\n", dlerror());
		exit(1);
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
