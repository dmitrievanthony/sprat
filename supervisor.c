#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "container.h"

static void check_arguments(int argc, char **argv);

int main(int argc, char **argv) {
	check_arguments(argc, argv);
	printf("Supervisor started\n");
	container *c = start_container(argv[1], argv[2]);
	waitpid(c->pid, NULL, 0);
	printf("Supervisor finished\n");
	return 0;
}

static void check_arguments(int argc, char **argv) {
	if (argc != 3) {
		printf("./sprat IMAGE_NAME INIT_SCRIPT\n");
		exit(-1);
	}
}
