#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/loop.h>

static char *find_free_loop_device();
static int attach_loop_device(char *loop_device_name, char *file_name);
static int detach_loop_device(char *loop_device_name);

/* Attaches the image file to a free loop device and then mounts this loop device to the target directory.*/
static char *mount_loopfs(char *img, char *target, char *fstype) {
	char *loop_device = find_free_loop_device();
	if (loop_device > 0) {
		if (attach_loop_device(loop_device, img) == 0) {
			if (mount(loop_device, target, fstype, 0, "") == 0) {
				return loop_device;
			}
			else {
				detach_loop_device(loop_device);
				fprintf(stderr, "Canot mount %s on %s\n", loop_device, target);
			}
		}
		else {
			detach_loop_device(loop_device);	
			fprintf(stderr, "Cannot attach %s to loop device %s\n", img, loop_device);
		}
	}
	else fprintf(stderr, "Cannot find free loop device\n");
	return (char *) -1;
}
 
/* Unmount the target directory and then detaches any file from the loop device. */
int umount_loop_filesystem(char *loop_device, char *target) {
	if (umount(target) == 0) {
		if (detach_loop_device(loop_device) == 0) return 0;
		else fprintf(stderr, "Cannot detach loop device %s\n", loop_device);
	}
	else fprintf(stderr, "Cannot umount %s\n", target);
	return -1;
}
 
#define LOOP_DEVICE_NAME_MAX_SIZE 20
static char *find_free_loop_device() {
	char *loop_device_name = malloc(LOOP_DEVICE_NAME_MAX_SIZE);
	int loop_control = open("/dev/loop-control", O_RDWR);
	if (loop_control > 0) {
		int free_loop_device_number = ioctl(loop_control, LOOP_CTL_GET_FREE);
        if (free_loop_device_number > 0) {
			sprintf(loop_device_name, "/dev/loop%d", free_loop_device_number);
			close(loop_control);
            return loop_device_name;
        }
        else fprintf(stderr, "Cannot find free loop device\n");
	}
	else fprintf(stderr, "Cannot open loop control (/dev/loop_control)\n");
	close(loop_control);
	free(loop_device_name);
	return (char *) -1;
}
 
static int attach_loop_device(char *loop_device, char *file) {
	int loop_device_fd = open(loop_device, O_RDWR);
	if (loop_device_fd > 0) {
		int file_fd = open(file, O_RDWR);
		if (file_fd > 0) {
			if (ioctl(loop_device_fd, LOOP_SET_FD, file) == 0) {	
				close(file_fd);
				close(loop_device_fd);
				return 0;
			}
			else fprintf(stderr, "Cannot attach %s to %s\n", file, loop_device);
		}
		else fprintf(stderr, "Cannot open file %s\n", file);
		close(file_fd);
	}
	else fprintf(stderr, "Cannot open loop device file %s\n", loop_device);
    close(loop_device_fd);
    return -1;
}
 
static int detach_loop_device(char *loop_device) {
	int loop_device_fd = open(loop_device, O_RDWR);
	if (loop_device_fd > 0) {
		if (ioctl(loop_device_fd, LOOP_CLR_FD) == 0) {
			close(loop_device_fd);
			return 0;
		}
		else fprintf(stderr, "Cannot detach file from loop device %s\n", loop_device);
	}
	else fprintf(stderr, "Cannot open loop device %s\n", loop_device);
	close(loop_device_fd);
	return -1;
}
