#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

size_t io_blocksize() {
    //获取系统的页面大小
    size_t Page_Size = getpagesize();
    if (Page_Size == 0) {
        return 4096; // 默认4096B
    }
    return Page_Size;
}

char *align_alloc(size_t size){
    // 分配内存并对齐到系统页面大小
    size_t page_size = io_blocksize();
    void *ptr;
    if (posix_memalign(&ptr, page_size, size) != 0) {
        return NULL;
    }
    return (char *)ptr;
}

void align_free(char *ptr) {
    free(ptr);
}

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
    
    char *buf = align_alloc(io_blocksize());
    if (buf == NULL) {
        perror("Error allocating memory");
        close(fd);
        return EXIT_FAILURE;
    }
    
    while(1) {
        int rd_bytes = read(fd, buf, io_blocksize());
        if(rd_bytes < 0) {
            perror("Error reading file");
            close(fd);
            align_free(buf);
            return EXIT_FAILURE;
        }
        if (rd_bytes == 0) {
            break; // EOF
        }
        int wt_bytes = write(STDOUT_FILENO, buf, rd_bytes);
        if (wt_bytes < 0) {
            perror("Error writing to stdout");
            close(fd);
            align_free(buf);
            return EXIT_FAILURE;
        }
    }
    
    close(fd);
    align_free(buf);
    return EXIT_SUCCESS;
} 