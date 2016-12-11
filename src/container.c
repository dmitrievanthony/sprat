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
#include <sys/syscall.h>

#include "util.h"
#include "loopfs.h"

struct container {
    char *base_image;
	char *fstype;
    char *init_script;
    char *id;
    long pid;
	char *image;
	char *mnt_dir;
	char *loop_device;
};

static int init_function(void *argv);
static int init_container_env(struct container *c);
static int destroy_container_env(struct container *c);
static int init_container(struct container *c);
static char *generate_container_id();

#define STACK_SIZE  1024 * 1024
#define CLONE_FLAGS SIGCHLD | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS
int run_container(char *base_image, char *fstype, char *init_script) {
	struct container c;
	c.base_image = base_image;
	c.fstype = fstype;
	c.init_script = init_script;
	c.id = generate_container_id();
	if (c.id != (char *) -1) {
		if (init_container_env(&c) == 0) {
			c.pid = clone(init_function, malloc(STACK_SIZE) + STACK_SIZE, CLONE_FLAGS, &c);
			if (c.pid != -1) {
				if (waitpid(c.pid, NULL, 0) == c.pid) {
				 	return destroy_container_env(&c);
				}
				else {
					fprintf(stderr, "Cannot wait child process with PID %ld (return code %d)\n", c.pid, errno);
				}
			}
			else {
				fprintf(stderr, "Cannot clone process (return code %d)\n", errno);
			}
			destroy_container_env(&c);
		}
	}
	return -1;
}

static int init_function(void *argv) {
	struct container *c = (struct container *) argv;
	printf("Container %s started (base image %s)\n", c->id, c->base_image);
	if (init_container(c) == 0) {
		if (system(c->init_script) == 0) {
			printf("Container %s finished\n", c->id);
			return 0;
		}
		else fprintf(stderr, "Container finished with error (return code %d)\n", errno);
	}
	return -1;
}

#define UUID_SIZE 37
static char *generate_container_id() {
	FILE *uuid_source = fopen("/proc/sys/kernel/random/uuid", "r");
	if (uuid_source != NULL) {
		char *container_id = malloc(UUID_SIZE);
		fgets(container_id, UUID_SIZE, uuid_source);
		if (errno == 0) {
			fclose(uuid_source);
			return container_id;
		}
		else {
			fprintf(stderr, "Cannot get content of /proc/sys/kernel/random/uuid (return code %d)\n", errno);
		}
		free(container_id);
		fclose(uuid_source);
	}
	else {
		fprintf(stderr, "Cannot fopen /proc/sys/kernel/random/uuid (return code %d)\n", errno);
	}
	return (char *) -1;
}

static int init_container_env(struct container *c) {
	c->image = malloc(strlen(c->id) + 5);
	c->mnt_dir = malloc(strlen(c->id) + 1);
	sprintf(c->image, ".%s.img", c->id);
	sprintf(c->mnt_dir, ".%s", c->id);
	if (copy_file(c->base_image, c->image) == 0) {
		if (mkdir(c->mnt_dir, S_IRWXU) == 0) {
			return 0;
		}
		else {
			fprintf(stderr, "Cannot create directory %s (return code %d)\n", c->mnt_dir, errno);
		}
	}
	free(c->image);
	free(c->mnt_dir);
	return -1;
}

static int destroy_container_env(struct container *c) {
	int rm_image_res = unlink(c->image);
	int rm_mnt_dir = rmdir(c->mnt_dir);
	if ((rm_image_res == 0 || rm_image_res == ENOENT) && (rm_mnt_dir == 0 || rm_mnt_dir == ENOENT)) {
		return 0;
	}
	else {
		if (rm_image_res != 0 && rm_image_res != ENOENT) {
			fprintf(stderr, "Cannot remove file %s (return code %d)\n", c->image, rm_image_res);
		}
		if (rm_mnt_dir != 0 && rm_mnt_dir != ENOENT) {
			fprintf(stderr, "Cannot remove directory %s (return code %d)\n", c->mnt_dir, rm_mnt_dir);
		}
	}
	return -1;
}

static int init_container(struct container *c) {
	c->loop_device = mount_loopfs(c->image, c->mnt_dir, c->fstype);
	if (c->loop_device != (char *) -1) {
		if (chdir(c->mnt_dir) == 0) {
			if (mkdir(".root", S_IRWXU) == 0) {
				chroot(".");
				// TODO: pivot_root always return 22 (Illegal argument)
				//
				// if (syscall(SYS_pivot_root, ".", OLD_ROOT_DIR) == 0) {
				// 	return chdir("/");
				// }
				// else fprintf(stderr, "Cannot change root (return code %d)\n", errno);
				return 0;
			}
			else {
				fprintf(stderr, "Cannot create directory .%s/.root (return code %d)\n", c->id, errno);
			}
		}
		else {
			fprintf(stderr, "Cannot change directory to %s (return code %d)\n", c->mnt_dir, errno);
		}
	}
	return -1;
}
