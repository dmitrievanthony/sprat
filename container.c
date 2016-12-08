#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>

#include "container.h"

static char *generate_container_id();
static int init_function(void *argv);

#define STACK_SIZE  1024 * 1024
#define CLONE_FLAGS CLONE_NEWPID | CLONE_NEWNS | SIGCHLD

container *start_container(char *image_name, char *init_script) {
	container *c = malloc(sizeof(container));
	c->image_name = image_name;
	c->init_script = init_script;
	c->id = generate_container_id();
	char *stack = malloc(STACK_SIZE);
	c->pid = clone(init_function, stack + STACK_SIZE, CLONE_FLAGS, c);
	return c;
}

static int init_function(void *argv) {
	container *c = (container *) argv;
	printf("Container \"%s\" started from image \"%s\" with init script \"%s\"\n", c->id, c->image_name, c->init_script);
	system("ls -la");
	printf("Container \"%s\" finished\n", c->id);
	return 0;
}

#define UUID_SIZE 37

static char *generate_container_id() {
	FILE *uuid_source = fopen("/proc/sys/kernel/random/uuid", "r");
	char *container_id = malloc(UUID_SIZE);
	fgets(container_id, UUID_SIZE, uuid_source);
	fclose(uuid_source);
	return container_id;
}
