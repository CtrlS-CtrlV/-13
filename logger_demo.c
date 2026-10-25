/* демонстрація втрати даних при різних режимах буферизації */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define LOG_LINES 20

static void write_log(const char *path, int buf_mode, const char *mode_name,
                      int crash)
{
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("fopen"); return; }

    char custom_buf[4096];
    setvbuf(fp, custom_buf, buf_mode, sizeof(custom_buf));

    for (int i = 1; i <= LOG_LINES; i++) {
        fprintf(fp, "[%s] Запис #%02d: лог-повідомлення\n", mode_name, i);

        if (i == LOG_LINES / 2 && crash) {
            // Аварійне завершення через _exit() без fflush/fclose
            _exit(0);
        }
    }

    fclose(fp);
}

static int count_lines(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp))
        count++;
    fclose(fp);
    return count;
}

int main(void)
{
    printf("=== Демонстрація втрати даних при різних режимах буферизації ===\n\n");

    struct {
        const char *name;
        int mode;
        const char *path;
    } tests[] = {
        { "_IOFBF (повна)",    _IOFBF, "/tmp/sp_log_iofbf.txt" },
        { "_IOLBF (рядкова)", _IOLBF, "/tmp/sp_log_iolbf.txt" },
        { "_IONBF (без буф.)", _IONBF, "/tmp/sp_log_ionbf.txt" },
    };
    int ntests = sizeof(tests) / sizeof(tests[0]);

    printf("Запис %d рядків, \"аварія\" після рядка %d\n\n", LOG_LINES, LOG_LINES/2);
    printf("%-20s   %10s   %10s\n", "Режим", "Записано", "Втрачено");
    printf("──────────────────────────────────────────────\n");

    for (int i = 0; i < ntests; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            write_log(tests[i].path, tests[i].mode, tests[i].name, 1);
            _exit(0);
        } else {
            waitpid(pid, NULL, 0);
            int lines = count_lines(tests[i].path);
            int expected_at_crash = LOG_LINES / 2;
            int lost = expected_at_crash - lines;
            if (lost < 0) lost = 0;
            printf("%-20s   %10d   %10d\n", tests[i].name, lines, lost);
        }
    }

    printf("\n--- Контрольний запис (без аварії) ---\n");
    for (int i = 0; i < ntests; i++) {
        write_log(tests[i].path, tests[i].mode, tests[i].name, 0);
        int lines = count_lines(tests[i].path);
        printf("%-20s   %10d рядків (успішно)\n", tests[i].name, lines);
    }

    printf("\n=== Висновок ===\n");
    printf("_IOFBF: втрачається весь вміст буфера (до 4 КБ даних).\n");
    printf("_IOLBF: втрата мінімальна (буфер скидається після кожного '\\n').\n");
    printf("_IONBF: втрати немає (кожен запис — негайний системний виклик).\n");
    printf("fflush() після критичних записів запобігає втраті.\n");

    for (int i = 0; i < ntests; i++) {
        unlink(tests[i].path);
    }

    return EXIT_SUCCESS;
}
