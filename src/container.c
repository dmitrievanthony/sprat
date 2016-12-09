#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "util.h"
#include "loopfs.h"

struct container {
    char *base_image;
    char *init_script;
    char *id;
    long pid;
	char *image;
	char *mount_dir;
};

static char *generate_container_id();
static int init_function(void *argv);
static int init_container_env(struct container *c);
static int destroy_container_env(struct container *c);
static int init_container(struct container *c);
static int destroy_container(struct container *c);

#define STACK_SIZE  1024 * 1024
#define CLONE_FLAGS SIGCHLD | CLONE_NEWPID | CLONE_NEWNS
int run_container(char *base_image, char *init_script) {
	struct container c;
	c.base_image = base_image;
	c.init_script = init_script;
	c.id = generate_container_id();
	if (init_container_env(&c) < 0) {
		fprintf(stderr, "Cannot initialize \"%s\" container environment\n", c.id);
	}
	c.pid = clone(init_function, malloc(STACK_SIZE) + STACK_SIZE, CLONE_FLAGS, &c);
	if (c.pid > 0) {
		if (waitpid(c.pid, NULL, 0) == 0) {
			destroy_container_env(&c);
			return 0;
		}
		else fprintf(stderr, "Cannot wait container \"%s\" process with PID %ld\n", c.id, c.pid);
	}
	else fprintf(stderr, "Cannot clone container \"%s\" process, return PID %ld\n", c.id, c.pid);
	destroy_container_env(&c);
	return -1;
}

static int init_function(void *argv) {
	struct container *c = (struct container *) argv;
	printf("Container \"%s\" started from base image \"%s\" with init script \"%s\"\n", c->id, c->base_image, c->init_script);
	if (init_container(c) == 0) {
		system(c->init_script);
		destroy_container(c);
		return 0;
	}
	else fprintf(stderr, "Cannot initialize container \"%s\"\n", c->id);
	return -1;
}

#define UUID_SIZE 37
static char *generate_container_id() {
	FILE *uuid_source = fopen("/proc/sys/kernel/random/uuid", "r");
	char *container_id = malloc(UUID_SIZE);
	fgets(container_id, UUID_SIZE, uuid_source);
	fclose(uuid_source);
	return container_id;
}

#define FILE_NAME_SIZE 100
#define SPRAT_DIR ".sprat"
#define SPRAT_DIR_MODE 666
#define SPRAT_MNT_DIR ".sprat/mnt"
#define SPRAT_MNT_DIR_MODE 666
static int init_container_env(struct container *c) {
	int create_sprat_dir_res = mkdir(SPRAT_DIR, SPRAT_DIR_MODE);
	int create_sprat_mount_dir_res = mkdir(SPRAT_MNT_DIR, SPRAT_MNT_DIR_MODE);
	if ((create_sprat_dir_res == 0 || create_sprat_dir_res == EEXIST) && 
		(create_sprat_mount_dir_res == 0 || create_sprat_mount_dir_res == EEXIST)) {
		c->image = malloc(FILE_NAME_SIZE);
		c->mount_dir = malloc(FILE_NAME_SIZE);
		sprintf(c->image, "%s/%s", SPRAT_DIR, c->id);
		sprintf(c->mount_dir, "%s/%s", SPRAT_MNT_DIR, c->id);
		if (copy_file(c->base_image, c->image) > 0) {
			if (mkdir(c->mount_dir, SPRAT_MNT_DIR_MODE) > 0) {
			}
			else fprintf(stderr, "Cannot create mount directory %s\n", c->mount_dir);
		}
		else fprintf(stderr, "Cannot copy base image %s to base image %s\n", c->base_image, c->image);
	}
	else fprintf(stderr, "Cannot create sprat directories\n"); 
	return -1;
}

static int destroy_container_env(struct container *c) {
	return 0;
}

static int init_container(struct container *c) {
	return 0;
}

static int destroy_container(struct container *c) {
	return 0;
}
