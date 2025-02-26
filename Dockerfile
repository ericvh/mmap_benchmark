# Dockerfile for mmap_benchmark

FROM gcc:latest

WORKDIR /app

COPY mmap_benchmark.c .

RUN gcc -o mmap_benchmark mmap_benchmark.c

CMD ["./mmap_benchmark"]