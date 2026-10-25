/* unbuf_copy.c — копіювання файлу через системні виклики з різними буферами */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static double elapsed_ms(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_nsec - start->tv_nsec) / 1e6;
}

static long long copy_file(const char *src_path, const char *dst_path,
                           size_t block_size)
{
    int fd_src = open(src_path, O_RDONLY);
    if (fd_src == -1) { perror("open src"); return -1; }

    int fd_dst = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dst == -1) {
        perror("open dst");
        close(fd_src);
        return -1;
    }

    char *buf = malloc(block_size);
    if (!buf) { perror("malloc"); close(fd_src); close(fd_dst); return -1; }

    long long total = 0;
    ssize_t bytes_read;

    while ((bytes_read = read(fd_src, buf, block_size)) > 0) {
        ssize_t written = 0;
        while (written < bytes_read) {
            ssize_t w = write(fd_dst, buf + written, bytes_read - written);
            if (w == -1) {
                perror("write");
                free(buf);
                close(fd_src);
                close(fd_dst);
                return -1;
            }
            written += w;
        }
        total += bytes_read;
    }

    if (bytes_read == -1) {
        perror("read");
        total = -1;
    }

    free(buf);
    close(fd_src);
    close(fd_dst);
    return total;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Використання: %s <файл_для_копіювання>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src = argv[1];
    const char *dst = "/tmp/sp_copy_test";
    size_t sizes[] = { 1, 512, 4096, 65536 };
    int n = sizeof(sizes) / sizeof(sizes[0]);

    printf("%-15s   %12s   %12s\n", "Розмір буфера", "Час (мс)", "Байтів");
    printf("─────────────────────────────────────────────\n");

    for (int i = 0; i < n; i++) {
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        long long bytes = copy_file(src, dst, sizes[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms = elapsed_ms(&t0, &t1);

        if (bytes != -1) {
            printf("%-15zu   %12.2f   %12lld\n", sizes[i], ms, bytes);
        } else {
            printf("%-15zu   %12s   %12s\n", sizes[i], "ПОМИЛКА", "-");
        }

        unlink(dst);
    }

    printf("\nВисновок: при малих блоках (1 байт) кількість системних\n"
           "викликів величезна, що різко уповільнює копіювання.\n");

    return EXIT_SUCCESS;
}
