#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>

#define FILE_SIZE (256 * 1024 * 1024) // 256MB
#define PAGE_SIZE 4096               // 4KB
#define NUM_ACCESSES (FILE_SIZE / PAGE_SIZE) // Number of accesses in sequential/random

// Get time in seconds
double get_time_in_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Sequential read benchmark
void benchmark_sequential(const char *mapped) {
    volatile char sum = 0;
    double start = get_time_in_seconds();

    for (size_t i = 0; i < FILE_SIZE; i += PAGE_SIZE) {
        sum += mapped[i];  // Accessing each page sequentially
    }

    double end = get_time_in_seconds();
    printf("Sequential read time: %f seconds\n", end - start);
}

// Random read benchmark
void benchmark_random(const char *mapped) {
    volatile char sum = 0;
    double start = get_time_in_seconds();

    for (size_t i = 0; i < NUM_ACCESSES; i++) {
        size_t rand_offset = (rand() % NUM_ACCESSES) * PAGE_SIZE;
        sum += mapped[rand_offset];  // Accessing a random page
    }

    double end = get_time_in_seconds();
    printf("Random read time: %f seconds\n", end - start);
}

void run_benchmark(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Running mmap benchmarks on: %s\n", filename);
    benchmark_sequential(mapped);
    benchmark_random(mapped);

    munmap(mapped, FILE_SIZE);
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
