# Makefile for mmap_benchmark.c

CC = gcc
CFLAGS = -Wall -O2
TARGET = mmap_benchmark
SRCS = mmap_benchmark.c

all: $(TARGET)

$(TARGET): $(SRCS)
    $(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
    rm -f $(TARGET)

.PHONY: all clean