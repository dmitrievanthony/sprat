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
#include <sys/mount.h>

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
static int destroy_container(struct container *c);
static char *generate_container_id();

#define STACK_SIZE  1024 * 1024
#define CLONE_FLAGS SIGCHLD | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS
int run_container(char *base_image, char *fstype, char *init_script) {
	struct container c;
	c.base_image = base_image;
	c.fstype = fstype;
	c.init_script = init_script;
	c.id = generate_container_id();
	if (c.id < 0 || init_container_env(&c) < 0) {
		return -1;
	}
	c.pid = clone(init_function, malloc(STACK_SIZE) + STACK_SIZE, CLONE_FLAGS, &c);
	if (c.pid < 0) {
		fprintf(stderr, "Cannot clone process (error code %d)\n", errno);
		destroy_container_env(&c);
		free(c.id);
		return -1;
	}
	if (waitpid(c.pid, NULL, 0) != c.pid) {
		fprintf(stderr, "Cannot wait child process with PID %ld (error code %d)\n", c.pid, errno);
		destroy_container_env(&c);
		free(c.id);
		return -1;
	}
	return destroy_container_env(&c);
}

static int init_function(void *argv) {
	struct container *c = (struct container *) argv;
	printf("Container %s started (base image %s)\n", c->id, c->base_image);
	if (init_container(c) != 0) {
		fprintf(stderr, "Container finished with error (error code %d)\n", errno);
		return -1;
	}
	if (system(c->init_script) != 0) {
		fprintf(stderr, "Container finished with error (error code %d)\n", errno);
		destroy_container(c);
		return -1;
	}
	destroy_container(c);
	return 0;
}

#define UUID_SIZE 37
static char *generate_container_id() {
	FILE *uuid_source = fopen("/proc/sys/kernel/random/uuid", "r");
	if (uuid_source == NULL) {
		fprintf(stderr, "Cannot fopen /proc/sys/kernel/random/uuid (error code %d)\n", errno);
		return (char *) -1;
	}
	char *container_id = malloc(UUID_SIZE + 1);
	fgets(container_id, UUID_SIZE, uuid_source);
	if (errno != 0) {
		fprintf(stderr, "Cannot get content of /proc/sys/kernel/random/uuid (error code %d)\n", errno);
		free(container_id);
		fclose(uuid_source);
		return (char *) -1;
	}
	fclose(uuid_source);
	return container_id;
}

static int init_container_env(struct container *c) {
	c->image = malloc(strlen(c->id) + 5);
	c->mnt_dir = malloc(strlen(c->id) + 1);
	sprintf(c->image, ".%s.img", c->id);
	sprintf(c->mnt_dir, ".%s", c->id);
	if (copy_file(c->base_image, c->image) != 0) {
		fprintf(stderr, "Cannot copy file %s to %s (error code %d)\n", c->base_image, c->image, errno);
		free(c->image);
		free(c->mnt_dir);
		return -1;
	}
	if (mkdir(c->mnt_dir, S_IRWXU) != 0) {
		fprintf(stderr, "Cannot create directory %s (error code %d)\n", c->mnt_dir, errno);
		free(c->image);
		free(c->mnt_dir);
		return -1;
	}
	return 0;
}

static int destroy_container_env(struct container *c) {
	int rm_image_res = unlink(c->image);
	int rm_mnt_dir = rmdir(c->mnt_dir);
	if ((rm_image_res != 0 && rm_image_res != ENOENT) || (rm_mnt_dir != 0 && rm_mnt_dir != ENOENT)) {
		if (rm_image_res != 0 && rm_image_res != ENOENT) {
            fprintf(stderr, "Cannot remove file %s (error code %d)\n", c->image, rm_image_res);
        }
		if (rm_mnt_dir != 0 && rm_mnt_dir != ENOENT) {
            fprintf(stderr, "Cannot remove directory %s (error code %d)\n", c->mnt_dir, rm_mnt_dir);
        }
		return -1;
	}
	return 0;
}

static int create_and_mount(const char *source, const char *target, const char *fstype, unsigned long flags, const void *data) {
	if (mkdir(target, S_IRWXU) != 0 && errno != EEXIST) {
		fprintf(stderr, "Cannot create directory %s (error code %d)\n", target, errno);
		return -1;
	}
	int new_directory = errno == EEXIST ? 0 : 1;
	if (mount(source, target, fstype, flags, data) != 0) {
		fprintf(stderr, "Cannot mount %s on %s with fstype %s (error code %d)\n", source, target, fstype, errno);
		if (new_directory) {
			rmdir(target);
		}
		return -1;
	}
	return 0;
}

static int create_symlink(const char *source, const char *target) {
	if (symlink(source, target) != 0 && errno != EEXIST) {
		fprintf(stderr, "Cannot create symlink %s --> %s (error code %d)\n", source, target, errno);
		return -1;
	}
	return 0;
}

static int init_container(struct container *c) {
	c->loop_device = mount_loopfs(c->image, c->mnt_dir, c->fstype);
	if (c->loop_device < 0) {
		return -1;
	}
	if (chdir(c->mnt_dir) < 0) {
		fprintf(stderr, "Cannot create directory .%s/.root (error code %d)\n", c->id, errno);
		return -1;
	}
	if (mkdir(".root", S_IRWXU) < 0 && errno != EEXIST) {
		fprintf(stderr, "Cannot create directory .%s/.root (error code %d)\n", c->id, errno);
		return -1;
	}
	if (chroot(".") < 0) {
		fprintf(stderr, "Cannot change directory (error code %d)\n", errno);
		return -1;
	}
	if (mkdir("/etc", S_IRWXU) != 0 && errno != EEXIST) {
		fprintf(stderr, "Cannot create directory /etc (error code %d)\n", errno);
		return -1;
	}
	if (!(create_and_mount("none", "/proc", "proc", MS_NOEXEC | MS_NOSUID | MS_NODEV, "") && 
		create_and_mount("none", "/dev", "tmpfs", MS_NOEXEC | MS_STRICTATIME, "mode=755") &&
		create_and_mount("none", "/dev/shm", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV, "mode=1777,size=65536k") &&
		create_and_mount("none", "/dev/mqueue", "mqueue", MS_NOEXEC | MS_NOSUID | MS_NODEV, "") &&
		create_and_mount("none", "/dev/pts", "devpts", MS_NOEXEC | MS_NOSUID, "newinstance,ptmxmode=0666,mode=620,gid=5") &&
		create_and_mount("none", "/sys", "sysfs", MS_NOEXEC | MS_NOSUID | MS_NODEV | MS_RDONLY, "") &&
		create_symlink("/proc/self/fd", "/dev/fd") &&
		create_symlink("/proc/self/fd/0", "/dev/stdin") &&
		create_symlink("/proc/self/fd/1", "/dev/stdout") &&
		create_symlink("/proc/self/fd/2", "/dev/stderr") &&
		create_symlink("/proc/mounts", "/etc/mtab"))) {
		return -1;
	}
	return 0;
}

static int destroy_container(struct container *c) {
	return umount_loopfs(c->loop_device, c->mnt_dir);
}

