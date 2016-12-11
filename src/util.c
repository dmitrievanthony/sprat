#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

int copy_file(char *src, char *dst) {
	struct stat st;
	int src_fd = open(src, O_RDONLY);
	if (src_fd != -1) {
		int dst_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		if (dst_fd != -1) {
			if (fstat(src_fd, &st) == 0) {
            	void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, src_fd, 0);
				if (mem != MAP_FAILED) {
					if (write(dst_fd, mem, st.st_size) == st.st_size) {
						munmap(mem, st.st_size);
						close(src_fd);
						close(dst_fd);
						return 0;
					}
					else {
						fprintf(stderr, "Cannot write file %s (error code %d)\n", dst, errno);
					}
				munmap(mem, st.st_size);
				}
				else {
					fprintf(stderr, "Cannot mmap file %s (error code %d)\n", src, errno);
				}
			}
			else {
				fprintf(stderr, "Cannot get size of file %s (return code %d)\n", src, errno);
			}
			close(dst_fd);
		}
		else {
			fprintf(stderr, "Cannot open file %s (error code %d)\n", dst, errno);
		}
		close(src_fd);
	}
	else {
		fprintf(stderr, "Cannot open file %s (error code %d)\n", src, errno);
	}
	return -1;
}
