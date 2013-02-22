#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_LOAD_TUNE 0.4
#define LOADAVG_PROC "/proc/loadavg"
#define lt_debug(fmt, args...) \
	do { \
		printf(">>>> [%s: %s, %d] " fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##args); \
	} while(0)
#undef lt_debug
#define lt_debug


static float loadtune = 0;
static int nrcpus = 0;
static int affinity = 0;

static float get_loadavg(void)
{
	static FILE * loadavg_fd = 0;
	char * lineptr = NULL; /* TODO static lineptr to avoid 
							  memory allocating and free, to improve 
							  peroforamance */
	size_t linelen = 0;
	float first = 0;
	char linedelim[] = " ";

	loadavg_fd = fopen(LOADAVG_PROC, "r");
	if (!loadavg_fd) {
		printf("Failed to open %s.\n", LOADAVG_PROC);
		return;
	}

	getline(&lineptr, &linelen, loadavg_fd);
	if (lineptr) {
		char * __lineptr = lineptr;
		first = atof(strsep(&__lineptr, linedelim));
		free(lineptr);
	}

	fclose(loadavg_fd);

	return first;
}

static void print_help(void)
{
	printf("Usage:\n");
	printf("    loadtune-daemon target-value\n");
	printf("Options:\n");
	printf("    For single core machine, target-value is between 0 and 1;\n");
	printf("    For multiple[N] core machine, target-value is between 0 and N.\n");
	printf("    (N is %d on this machine)\n", nrcpus);
	printf("Example:\n");
	printf("    loadtune-daemon 0.4\n");
}

static void tune_proc(void * arg);
static void tune_proc_start(void)
{
	pthread_t thread;
	affinity++;
	pthread_create(&thread, NULL, (void *)tune_proc, NULL);
	pthread_join(thread, NULL);
	return;
}

static void tune_proc(void * arg)
{
	pthread_t thread;
	cpu_set_t cpuset;

	lt_debug("affinity: %d.\n", affinity);
	thread = pthread_self();
	lt_debug("pthread: %d.\n", thread);
	CPU_ZERO(&cpuset);
	CPU_SET(affinity % nrcpus, &cpuset);
	pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	while (get_loadavg() < loadtune){
		lt_debug("pthread: %d.\n", thread);
		lt_debug("current load average: %f.\n", get_loadavg());
		lt_debug("target load average: %f.\n", loadtune);
	}
	lt_debug("end of tune_proc.\n");
	return;
}

void main(int argc, char ** argv)
{
	float current_load = 0;
	int proc_index = 0;

	/* get processor cnt */
	nrcpus = sysconf(_SC_NPROCESSORS_ONLN);
	lt_debug("nrcpus: %d.\n", nrcpus);
	lt_debug("pid: %d.\n", getpid());

	/* parameters validation */
	if (argc == 2) {
		int i = 0;
		if ((strcmp(argv[1], "--help") == 0) 
				|| ((strcmp(argv[1], "-h") == 0))) {
			print_help();
			return;
		}
		for (; argv[1][i] != '\0'; i++) {
			if (isdigit(argv[1][i]) || (argv[1][i] == '.')) {
				continue;
			}
			else {
				print_help();
				return;
			}
		}

		loadtune = atof(argv[1]);
	}
	else {
		printf("Invalid parameter count.\n");
		print_help();
		return;
	}
	lt_debug("loadtune: %f.\n", loadtune);
	if (loadtune > (float)(nrcpus)) {
		printf("Invalid target-value, it MUST be lower than nrcpus.\n");
		print_help();
	}

	while(1) {
		lt_debug("current load average: %f.\n", get_loadavg());
		lt_debug("target load average: %f.\n", loadtune);
		if (get_loadavg() < loadtune) {
			tune_proc_start();
		}
	}

	return;
}
