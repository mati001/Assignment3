CC = gcc
CFLAGS = -O2 -fopenmp -Wall
LDFLAGS = -lm

all: gauss_bench

gauss_bench: main.c guassonFilter.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f gauss_bench
