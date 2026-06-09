#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        perror("open source");
        return 1;
    }

    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("open dest");
        close(src_fd);
        return 1;
    }

    char buf[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(src_fd, buf, BUFFER_SIZE)) > 0) {
        char *p = buf;
        ssize_t remaining = bytes_read;
        while (remaining > 0) {
            bytes_written = write(dst_fd, p, remaining);
            if (bytes_written < 0) {
                perror("write");
                close(src_fd);
                close(dst_fd);
                return 1;
            }
            p += bytes_written;
            remaining -= bytes_written;
        }
    }

    if (bytes_read < 0) {
        perror("read");
        close(src_fd);
        close(dst_fd);
        return 1;
    }

    printf("Copied successfully.\n");

    close(src_fd);
    close(dst_fd);
    return 0;
}
