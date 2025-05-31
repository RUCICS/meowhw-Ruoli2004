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
    
    // 获取文件系统块大小
    struct stat st;
    size_t fs_block_size = page_size; // 默认使用页面大小
    
    if (fstat(fd, &st) == 0 && st.st_blksize > 0) {
        fs_block_size = st.st_blksize;
        
        // 处理注意事项2：检查是否为2的幂，如果不是则调整
        if (!is_power_of_two(fs_block_size)) {
            fs_block_size = next_power_of_two(fs_block_size);
        }
        
        // 限制文件系统块大小在合理范围内
        if (fs_block_size < 4096) {
            fs_block_size = 4096;
        } else if (fs_block_size > 16384) {
            fs_block_size = 16384;
        }
    }
    
    // 选择较大的值
    size_t optimal_size = (fs_block_size > page_size) ? fs_block_size : page_size;
    
    // 确保最终大小在合理范围内
    if (optimal_size > 16384) {
        optimal_size = 16384;
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
    
    // 根据文件描述符计算最优缓冲区大小
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