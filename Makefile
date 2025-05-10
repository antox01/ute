CC=gcc

OUTDIR=out
IFLAGS=-I./include
CFLAGS=-Wall -Wextra -lncurses

OUT_FILES=out/line.o out/buffer.o

all: $(OUTDIR) ute

.PHONY=ute
ute: src/main.c $(OUT_FILES)
	$(CC) -g -o out/ute src/main.c $(OUT_FILES) $(CFLAGS) $(IFLAGS)

out/line.o: src/line.c
	$(CC) -g -c -o out/line.o src/line.c $(IFLAGS)

out/buffer.o: src/buffer.c
	$(CC) -g -c -o out/buffer.o src/buffer.c $(IFLAGS)

$(OUTDIR):
	mkdir $(OUTDIR)
