#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/loop.h>

#include "loopfs.h"

static char *find_free_loop_device();
static int attach_loop_device(char *loop_device_name, char *file_name);

char *mount_loopfs(char *img, char *target, char *fstype) {
	char *loop_device = find_free_loop_device();
	if (loop_device < 0) {
		return (char *) -1;
	}
	if (attach_loop_device(loop_device, img) != 0) {
		free(loop_device);
		return (char *) -1;
	}
	if (mount(loop_device, target, fstype, 0, "") != 0) {
		fprintf(stderr, "Canot mount %s on %s with fstype %s (error code %d)\n", loop_device, target, fstype, errno);
		detach_loop_device(loop_device);
		free(loop_device);
		return (char *) -1;
	}
	return loop_device;
}
 
int umount_loopfs(char *target) {
	if (umount(target) != 0) {
		fprintf(stderr, "Cannot umount %s (error code %d)\n", target, errno);
		return -1;
	}
	return 0;
}

int detach_loop_device(char *loop_device) {
        int loop_device_fd = open(loop_device, O_RDWR);
        if (loop_device_fd < 0) {
                fprintf(stderr, "Cannot open loop device %s (error code %d)\n", loop_device, errno);
                return -1;
        }
        if (ioctl(loop_device_fd, LOOP_CLR_FD) != 0) {
                fprintf(stderr, "Cannot detach file from loop device %s (error code %d)\n", loop_device, errno);
                close(loop_device_fd);
                return -1;
        }
        close(loop_device_fd);
        return 0;
}
 
#define LOOP_DEVICE_NAME_MAX_SIZE 20
static char *find_free_loop_device() {
	int loop_control_fd = open("/dev/loop-control", O_RDWR);
	if (loop_control_fd < 0) {
		fprintf(stderr, "Cannot open /dev/loop_control (error code %d)\n", errno);		
		return (char *) -1;
	}
	char *loop_device_name = malloc(LOOP_DEVICE_NAME_MAX_SIZE);
	int free_loop_device_number = ioctl(loop_control_fd, LOOP_CTL_GET_FREE);
	if (free_loop_device_number < 0) {
		fprintf(stderr, "Cannot find free loop device (error code %d)\n", errno);
		free(loop_device_name);
		return (char *) -1;
	}
	sprintf(loop_device_name, "/dev/loop%d", free_loop_device_number);
	close(loop_control_fd);
    return loop_device_name;
}
 
static int attach_loop_device(char *loop_device, char *file) {
	int loop_device_fd = open(loop_device, O_RDWR);
	if (loop_device_fd < 0) {
		fprintf(stderr, "Cannot open loop device %s (error code %d)\n", loop_device, errno);
		return -1;
	}
	int file_fd = open(file, O_RDWR);
	if (file_fd < 0) {
		fprintf(stderr, "Cannot open file %s (error code %d)\n", file, errno);
		close(loop_device_fd);
		return -1;
	}
	if (ioctl(loop_device_fd, LOOP_SET_FD, file_fd) != 0) {	
		fprintf(stderr, "Cannot attach file %s to loop device %s (error code %d)\n", file, loop_device, errno);
		close(loop_device_fd);
		close(file_fd);
		return -1;
	}
	close(file_fd);
	close(loop_device_fd);
	return 0;
}
