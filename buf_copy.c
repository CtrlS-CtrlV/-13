/* buf_copy.c — копіювання через stdio + дослідження setvbuf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double elapsed_ms(struct timespec *start, struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_nsec - start->tv_nsec) / 1e6;
}

static long long copy_buffered(const char *src_path, const char *dst_path,
                               size_t block_size, int buf_mode, size_t buf_size)
{
    FILE *fsrc = fopen(src_path, "rb");
    if (!fsrc) { perror("fopen src"); return -1; }

    FILE *fdst = fopen(dst_path, "wb");
    if (!fdst) { perror("fopen dst"); fclose(fsrc); return -1; }

    char *custom_buf = NULL;
    if (buf_size > 0) {
        custom_buf = malloc(buf_size);
        if (custom_buf) {
            setvbuf(fdst, custom_buf, buf_mode, buf_size);
        }
    } else {
        setvbuf(fdst, NULL, buf_mode, 0);
    }

    char *block = malloc(block_size);
    if (!block) {
        if (custom_buf) free(custom_buf);
        fclose(fdst);
        fclose(fsrc);
        return -1;
    }

    long long total = 0;
    size_t n;

    while ((n = fread(block, 1, block_size, fsrc)) > 0) {
        size_t written = fwrite(block, 1, n, fdst);
        if (written != n) {
            perror("fwrite");
            break;
        }
        total += n;
    }

    free(block);
    fclose(fdst);
    fclose(fsrc);
    if (custom_buf) free(custom_buf);
    return total;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Використання: %s <файл>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src = argv[1];
    const char *dst = "/tmp/sp_bufcopy_test";

    printf("=== Порівняння режимів буферизації stdio ===\n\n");

    struct {
        const char *name;
        int mode;
        size_t buf_size;
    } configs[] = {
        { "_IOFBF (8 КБ)",    _IOFBF, 8192  },
        { "_IOFBF (64 КБ)",   _IOFBF, 65536 },
        { "_IOLBF (8 КБ)",    _IOLBF, 8192  },
        { "_IONBF (без буф.)", _IONBF, 0     },
    };
    int nconf = sizeof(configs) / sizeof(configs[0]);
    size_t block = 512;

    printf("Блок читання/запису: %zu байт\n\n", block);
    printf("%-22s   %12s\n", "Режим буферизації", "Час (мс)");
    printf("────────────────────────────────────────\n");

    for (int i = 0; i < nconf; i++) {
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        copy_buffered(src, dst, block, configs[i].mode, configs[i].buf_size);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms = elapsed_ms(&t0, &t1);

        printf("%-22s   %12.2f\n", configs[i].name, ms);

        remove(dst);
    }

    printf("\nВисновок: _IONBF найповільніший (кожен fwrite = системний виклик),\n"
           "_IOFBF з великим буфером — найшвидший.\n");

    return EXIT_SUCCESS;
}
