#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

int copy_file(char *src, char *dst) {
	struct stat st;
	int src_fd = open(src, O_RDONLY);
	int dst_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (src_fd > 0 && dst_fd > 0) {
		if (fstat(src_fd, &st) == 0) {
			void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, src_fd, 0);
			if (mem > 0) {
				if (write(dst_fd, mem, st.st_size) == st.st_size) {
					munmap(mem, st.st_size);
					close(src_fd);
					close(dst_fd);
					return 0;
				}
				else fprintf(stderr, "Cannot write file %s\n", dst);
			}
			else fprintf(stderr, "Cannot mmap file %s\n", src);
		}
		else fprintf(stderr, "Cannot get size of file %s\n", src);
	}
	if (src_fd < 0) fprintf(stderr, "Cannot open file %s\n", src);
	if (dst_fd < 0) fprintf(stderr, "Cannot open file %s\n", src);
	close(src_fd);
	close(dst_fd);
	return -1;
}
