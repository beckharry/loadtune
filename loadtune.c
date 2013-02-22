#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#define DEFAULT_LOAD_TUNE 0.4
#define LOADAVG_PROC "/proc/loadavg"
#define lt_debug(fmt, args...) \
	do { \
		printf("[%s: %s, %d] " fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##args); \
	} while(0)

static float get_first_float(FILE * fd)
{
	char * lineptr = NULL; /* TODO static lineptr to avoid 
							  memory allocating and free, to improve 
							  peroforamance */
	size_t linelen = 0;
	float first = 0;
	char linedelim[] = " ";
	getline(&lineptr, &linelen, fd);
	if (lineptr) {
		char * __lineptr = lineptr;
		first = atof(strsep(&__lineptr, linedelim));
		free(lineptr);
	}
	return first;
}

static void print_help(void)
{
	int processor_cnt = 0;
	processor_cnt = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Usage:\n");
	printf("    loadtune target-value\n");
	printf("Options:\n");
	printf("    For single core machine, target-value is between 0 and 1;\n");
	printf("    For multiple[N] core machine, target-value is between 0 and N.\n");
	printf("    (N is %d on this machine)\n", processor_cnt);
	printf("Example:\n");
	printf("    loadtune 0.4\n");
}

void main(int argc, char ** argv)
{
	float loadtune = 0;
	FILE * loadavg_fd = 0;
	float current_load = 0;
	int processor_cnt = 0;

	/* get processor cnt */
	processor_cnt = sysconf(_SC_NPROCESSORS_ONLN);
	lt_debug("processor_cnt: %d.\n", processor_cnt);

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
		print_help();
	}
	lt_debug("loadtune: %f.\n", loadtune);
	if (loadtune > (float)(processor_cnt)) {
		printf("Invalid target-value, it MUST be lower than processor_cnt, see: loadtune --help\n");
	}

	/* get loadavg proc file handle */
	loadavg_fd = fopen(LOADAVG_PROC, "r");
	if (!loadavg_fd) {
		printf("Failed to open %s.\n", LOADAVG_PROC);
		return;
	}

	current_load = get_first_float(loadavg_fd);
	lt_debug("current_load: %f.\n", current_load);

	/* release loadavg proc file handle */
	fclose(loadavg_fd);

	return;
}
