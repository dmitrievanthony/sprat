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

struct container_env {
    char *image;
    char *mnt_dir;
	char *loop_device;
};

struct container {
    char *base_image;
	char *fstype;
    char *init_script;
    char *id;
    long pid;
	struct container_env *env;
};

static int init_function(void *argv);
static int init_container_env(struct container *c);
static int destroy_container_env(struct container *c);
static int init_container(struct container *c);
static char *generate_container_id();

#define STACK_SIZE  1024 * 1024
#define CLONE_FLAGS SIGCHLD | CLONE_NEWPID | CLONE_NEWNS
int run_container(char *base_image, char *fstype, char *init_script) {
	struct container c;
	c.base_image = base_image;
	c.fstype = fstype;
	c.init_script = init_script;
	c.id = generate_container_id();
	if (init_container_env(&c) < 0) {
		fprintf(stderr, "Cannot initialize %s container environment\n", c.id);
		return -1;
	}
	c.pid = clone(init_function, malloc(STACK_SIZE) + STACK_SIZE, CLONE_FLAGS, &c);
	if (c.pid > 0) {
		if (waitpid(c.pid, NULL, 0) == 0) {
			return destroy_container_env(&c);
		}
		else fprintf(stderr, "Cannot wait container %s process with PID %ld\n", c.id, c.pid);
	}
	else fprintf(stderr, "Cannot clone container %s process, return PID %ld\n", c.id, c.pid);
	destroy_container_env(&c);
	return -1;
}

static int init_function(void *argv) {
	struct container *c = (struct container *) argv;
	printf("Container %s started from base image %s with init script %s\n", c->id, c->base_image, c->init_script);
	if (init_container(c) == 0) {
		return system(c->init_script);
	}
	else fprintf(stderr, "Cannot initialize container %s\n", c->id);
	return -1;
}

/* Generates container ID base on UUID. */
#define UUID_SIZE 37
static char *generate_container_id() {
	FILE *uuid_source = fopen("/proc/sys/kernel/random/uuid", "r");
	char *container_id = malloc(UUID_SIZE);
	fgets(container_id, UUID_SIZE, uuid_source);
	fclose(uuid_source);
	return container_id;
}

/* Initializes container from supervisor process perspective (should be called before clone() in parent process), 
   creates runtime image and container folder. */
#define CONTAINER_MNT_DIR_MODE 666
static int init_container_env(struct container *c) {
	c->env = malloc(sizeof(struct container_env));
	c->env->image = malloc(strlen(c->id) + 5);
	c->env->mnt_dir = malloc(strlen(c->id) + 1);
	sprintf(c->env->image, ".%s.img", c->id);
	sprintf(c->env->mnt_dir, ".%s", c->id);
	int copy_img_res = copy_file(c->base_image, c->env->image);
	int mk_mnt_dir_res = mkdir(c->env->mnt_dir, CONTAINER_MNT_DIR_MODE);
	return copy_img_res == 0 && (mk_mnt_dir_res == 0 || mk_mnt_dir_res == EEXIST) ? 0 : -1;
}

/* Destroys container artifacts: runtime image and container folder (should be called in parent process). */
static int destroy_container_env(struct container *c) {
	int rm_image_res = unlink(c->env->image);
	int rm_mnt_dir = rmdir(c->env->mnt_dir);
	return ((rm_image_res == 0 || rm_image_res == ENOENT) && 
		(rm_mnt_dir == 0 || rm_mnt_dir == ENOENT)) ? 0 : -1;
}

/* Initializes container in child process (with new namespaces, etc..), attaches loop filesystem of runtime 
   image to the container directory and changes root to the container directory after that. */
#define OLD_ROOT_DIR ".old_root"
#define OLD_ROOT_DIR_MODE 666
static int init_container(struct container *c) {
	c->env->loop_device = mount_loopfs(c->env->image, c->env->mnt_dir, c->fstype);
	if (c->env->loop_device > 0) {
		if (chdir(c->env->mnt_dir) == 0) {
			int mk_old_root_dir_res = mkdir(OLD_ROOT_DIR, OLD_ROOT_DIR_MODE);
			if (mk_old_root_dir_res == 0 || mk_old_root_dir_res == EEXIST) {
				if (syscall(SYS_pivot_root, ".", OLD_ROOT_DIR) == 0) {
					return chdir("/");
				}
				else fprintf(stderr, "Cannot change root (return code %d)\n", errno);
			}
			else fprintf(stderr, "Cannot create directory .%s/%s\n", c->id, OLD_ROOT_DIR);
		}
		else fprintf(stderr, "Cannot change directory to %s\n", c->env->mnt_dir);
	}
	else fprintf(stderr, "Cannot mount loop filesystem\n");
	return -1;
}
