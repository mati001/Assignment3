CC = gcc
CFLAGS = -O2 -fopenmp -Wall
LDFLAGS = -lm

all: kmeans_bench

kmeans_bench: main.c kmeans.c dataset.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f kmeans_bench
