char *mount_loopfs(char *img, char* target, char *fstype);
int umount_loopfs(char *target);
int detach_loop_device(char *loop_device);
