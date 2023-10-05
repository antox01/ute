CC=gcc

IFLAGS=-I./include
CFLAGS=-Wall -lncurses

OUT_FILES=out/line.o out/buffer.o

.PHONY=setup

ute: setup src/main.c $(OUT_FILES)
	$(CC) -g -o out/ute src/main.c $(OUT_FILES) $(CFLAGS) $(IFLAGS)

out/line.o: src/line.c
	$(CC) -g -c -o out/line.o src/line.c $(IFLAGS)

out/buffer.o: src/buffer.c
	$(CC) -g -c -o out/buffer.o src/buffer.c $(IFLAGS)

setup:
	[ -d "out" ] || mkdir out
