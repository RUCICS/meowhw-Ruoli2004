#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    char buf;
    
    while(1) {
        int rd_bytes = read(fd, &buf, 1);
        if(rd_bytes < 0) {
            perror("Error reading file");
            close(fd);
            return EXIT_FAILURE;
        }
        if (rd_bytes == 0) {
            break; // EOF
        }
        int wt_bytes = write(STDOUT_FILENO, &buf, 1);
        if (wt_bytes < 0) {
            perror("Error writing to stdout");
            close(fd);
            return EXIT_FAILURE;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}