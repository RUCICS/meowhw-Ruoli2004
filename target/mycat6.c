#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

// 检查一个数是否为2的幂
int is_power_of_two(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// 找到大于等于n的最小的2的幂
size_t next_power_of_two(size_t n) {
    if (n <= 1) return 1;
    
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if (sizeof(size_t) > 4) {
        n |= n >> 32;
    }
    n++;
    
    return n;
}

size_t io_blocksize(int fd) {
    // 获取系统页面大小
    size_t page_size = getpagesize();
    if (page_size == 0) {
        page_size = 4096; // 默认4096B
    }
    
    // 简化：不考虑每个文件的不同块大小，使用固定的文件系统块大小
    size_t base_size = page_size; // 使用页面大小作为基础
    
    // 根据实验结果应用倍数优化
    size_t optimal_multiplier = 64;  
    size_t optimal_size = base_size * optimal_multiplier;
    
    // 限制在合理范围内：最小32KB，最大512KB
    if (optimal_size < 32768) {
        optimal_size = 32768;   // 32KB
    } else if (optimal_size > 524288) {
        optimal_size = 524288;  // 512KB
    }
    
    return optimal_size;
}

char *align_alloc(size_t size) {
    // 分配内存并对齐到系统页面大小
    size_t page_size = getpagesize();
    if (page_size == 0) {
        page_size = 4096;
    }
    
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
    
    //调用fadvise
    if(posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) < 0) {
        perror("Error setting file advice");
        close(fd);
        return EXIT_FAILURE;
    }
    // 计算最优缓冲区大小
    size_t buffer_size = io_blocksize(fd);
    
    char *buf = align_alloc(buffer_size);
    if (buf == NULL) {
        perror("Error allocating memory");
        close(fd);
        return EXIT_FAILURE;
    }
    
    while(1) {
        ssize_t rd_bytes = read(fd, buf, buffer_size);
        if(rd_bytes < 0) {
            perror("Error reading file");
            close(fd);
            align_free(buf);
            return EXIT_FAILURE;
        }
        if (rd_bytes == 0) {
            break; // EOF
        }
        ssize_t wt_bytes = write(STDOUT_FILENO, buf, rd_bytes);
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