#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "container.h"

static void check_arguments(int argc, char **argv);
static void check_capabilities();

int main(int argc, char **argv) {
	check_capabilities();
	check_arguments(argc, argv);
	printf("Supervisor started\n");
	if (run_container(argv[1], "ext4", argv[2]) == 0) {
		printf("Supervisor finished\n");
		return 0;
	}
	else fprintf(stderr, "Cannot start container\n");
	return -1;
}

static void check_arguments(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "./sprat IMAGE_NAME INIT_SCRIPT\n");
		exit(-1);
	}
}

static void check_capabilities() {
	if (getuid() != 0) {
		fprintf(stderr, "You haven't required permissions to make this action\n");
		exit(-1);
	}
}
