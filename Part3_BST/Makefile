CC = gcc
CFLAGS = -O2 -fopenmp -Wall
LDFLAGS = -lm

all: bst_bench

bst_bench: main.c binary_tree.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f bst_bench
