/* record_db.c — файл як база даних з довільним доступом */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_NAME 64

typedef struct {
    int    id;
    char   name[MAX_NAME];
    double score;
} Record;

static int append_record(int fd, const Record *rec)
{
    if (lseek(fd, 0, SEEK_END) == (off_t)-1) {
        perror("lseek SEEK_END");
        return -1;
    }
    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        perror("lseek SEEK_CUR");
        return -1;
    }
    int index = (int)(pos / sizeof(Record));

    if (write(fd, rec, sizeof(Record)) != sizeof(Record)) {
        perror("write");
        return -1;
    }
    return index;
}

static int read_record(int fd, int index, Record *rec)
{
    off_t offset = (off_t)index * sizeof(Record);
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek SEEK_SET");
        return -1;
    }
    ssize_t n = read(fd, rec, sizeof(Record));
    if (n != sizeof(Record)) {
        if (n == -1) perror("read");
        else fprintf(stderr, "read_record: неповний читаний блок (EOF)\n");
        return -1;
    }
    return 0;
}

static int update_record(int fd, int index, const Record *rec)
{
    off_t offset = (off_t)index * sizeof(Record);
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek SEEK_SET");
        return -1;
    }
    if (write(fd, rec, sizeof(Record)) != sizeof(Record)) {
        perror("write update");
        return -1;
    }
    return 0;
}

int main(void)
{
    const char *dbpath = "/tmp/sp_records.db";
    int fd = open(dbpath, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }

    printf("=== Демонстрація файлу як бази даних записів ===\n\n");

    Record samples[] = {
        { 1, "Шевченко Тарас",    95.5 },
        { 2, "Франко Іван",      88.0 },
        { 3, "Леся Українка",    92.3 },
        { 4, "Котляревський І.", 79.8 },
        { 5, "Сковорода Григорій", 91.0 }
    };

    for (int i = 0; i < 5; i++) {
        int idx = append_record(fd, &samples[i]);
        printf("Додано запис ID=%d із ім'ям '%s' на індекс %d\n", samples[i].id, samples[i].name, idx);
    }

    printf("\nЧитання запису з індексом 2:\n");
    Record r2;
    if (read_record(fd, 2, &r2) == 0) {
        printf("  [Index 2] ID: %d, Name: %s, Score: %.1f\n", r2.id, r2.name, r2.score);
    }

    printf("\nОновлення запису з індексом 1 (нова оцінка 99.0):\n");
    Record r1;
    if (read_record(fd, 1, &r1) == 0) {
        r1.score = 99.0;
        update_record(fd, 1, &r1);
    }

    printf("\nПеречитане оновлене значення для індексу 1:\n");
    Record r1_updated;
    if (read_record(fd, 1, &r1_updated) == 0) {
        printf("  [Index 1] ID: %d, Name: %s, Score: %.1f\n", r1_updated.id, r1_updated.name, r1_updated.score);
    }

    close(fd);
    unlink(dbpath);
    return EXIT_SUCCESS;
}
