CC=gcc

OUTDIR=out
IFLAGS=-I./include
CFLAGS=-Wall -Wextra

LD_FLAGS=-lncurses
# OUT_FILES+=out/line.o
OUT_FILES+=out/buffer.o
OUT_FILES+=out/main.o

all: $(OUTDIR) ute

.PHONY=ute
ute: out/ute

out/ute: $(OUT_FILES)
	$(CC) -g -o out/ute $(OUT_FILES) $(LD_FLAGS)

out/main.o: src/main.c
	$(CC) -g -c -o out/main.o src/main.c $(CFLAGS) $(IFLAGS)

out/line.o: src/line.c
	$(CC) -g -c -o out/line.o src/line.c $(CFLAGS) $(IFLAGS)

out/buffer.o: src/buffer.c include/buffer.h
	$(CC) -g -c -o out/buffer.o src/buffer.c $(CFLAGS) $(IFLAGS)

$(OUTDIR):
	mkdir -p $(OUTDIR)
