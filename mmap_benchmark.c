#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

#define PAGE_SIZE 4096 // 4KB
#define NUM_ITERATIONS 5

double get_time_in_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Get file size
size_t get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    return st.st_size;
}

void write_csv_header(FILE *csv_file) {
    fprintf(csv_file, "Path,File Size (bytes),Memory Size (bytes),Test Type,Time (seconds)\n");
}

void write_csv_record(FILE *csv_file, const char *path, size_t file_size, size_t memory_size, const char *test_type, double time) {
    fprintf(csv_file, "%s,%zu,%zu,%s,%f\n", path, file_size, memory_size, test_type, time);
}

// Sequential read benchmark
void benchmark_sequential(const char *mapped, size_t file_size, FILE *csv_file, const char *path) {
    volatile char sum = 0;
    double start = get_time_in_seconds();

    for (size_t i = 0; i < file_size; i += PAGE_SIZE) {
        sum += mapped[i];  // Accessing each page sequentially
    }

    double end = get_time_in_seconds();
    double time_taken = end - start;
    write_csv_record(csv_file, path, file_size, file_size, "Sequential", time_taken);
}

// Random read benchmark
void benchmark_random(const char *mapped, size_t num_accesses, FILE *csv_file, const char *path, size_t file_size) {
    volatile char sum = 0;
    double start = get_time_in_seconds();

    for (unsigned long long i = 0; i < num_accesses; i++) {
        size_t rand_offset = (rand() % num_accesses) * PAGE_SIZE;
        sum += mapped[rand_offset];  // Accessing a random page
    }

    double end = get_time_in_seconds();
    double time_taken = end - start;
    write_csv_record(csv_file, path, file_size, num_accesses * PAGE_SIZE, "Random", time_taken);
}

// Clear page cache
void clear_page_cache() {
    if (system("sh -c 'echo 3 > /proc/sys/vm/drop_caches'") != 0) {
        perror("Failed to clear page cache");
        exit(EXIT_FAILURE);
    }
}

void run_benchmark(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    size_t file_size = get_file_size(fd);
    size_t num_accesses = file_size / PAGE_SIZE;

    char *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    FILE *csv_file = fopen("benchmark_results.csv", "w");
    if (!csv_file) {
        perror("fopen");
        munmap(mapped, file_size);
        close(fd);
        exit(EXIT_FAILURE);
    }

    write_csv_header(csv_file);
    printf("Running mmap benchmarks on: %s\n", filename);

    for(int iteration=0; iteration<NUM_ITERATIONS; iteration++) {
        printf("Iteration %d\n", iteration + 1);
        clear_page_cache();
        benchmark_sequential(mapped, file_size, csv_file, filename);
        clear_page_cache();
        benchmark_random(mapped, num_accesses, csv_file, filename, file_size);
    }

    fclose(csv_file);
    munmap(mapped, file_size);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL)); // Seed for randomization
    run_benchmark(argv[1]);

    return EXIT_SUCCESS;
}
