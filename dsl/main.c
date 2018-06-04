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
#define SO_NAME "libdsl.so"

void create_file() {
	const char *src = "//extern \"C\" {\n"
			  "	#include <stdio.h>\n"
			  "	void _init() {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "	void _fini() {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "	__attribute__((constructor)) void constructor() {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "	__attribute__((destructor)) void destructor() {\n"
			  "		printf(\"%s\\n\", __func__);\n"
			  "	}\n"
			  "	double dsl_run(double d) {\n"
			  "		return d + (long)d;\n"
			  "	}\n"
			  "//}\n";
	int fd = open(FILE_NAME, O_CREAT|O_WRONLY|O_CLOEXEC, S_IWUSR|S_IRUSR);
	write(fd, src, strlen(src));
	close(fd);
}

void compile() {
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
		execlp("gcc", "gcc", "-shared", "-nostartfiles", soname, "-o", "so/" SO_NAME, OBJ_NAME, NULL);
	}
}

void *g_handle = NULL;
double (*g_dsl_fun)(double) = NULL;

void load() {
	g_handle = dlopen("so/" SO_NAME, RTLD_LAZY);
	if (g_handle == NULL) {
		printf("dlopen error: %s\n", dlerror());
		exit(1);
	}
	dlerror();
	g_dsl_fun = (double (*)(double))dlsym(g_handle, "dsl_run");
	if (g_dsl_fun == NULL) {
		printf("dlsym error: %s\n", dlerror());
	}
}

int main(int argc, char *argv[]) {
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

	printf("timecost: %ld.%03ld, g_dsl_fun=%p\n", (long)ts1.tv_sec, (long)ts1.tv_nsec/1000000, g_dsl_fun);

	double d = 1.18;
	printf("    %lf -> %lf\n", d, g_dsl_fun(d)); 

	dlclose(g_handle);
}
