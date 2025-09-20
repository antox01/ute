CC=gcc

OUTDIR=out
IFLAGS=-I./include
CFLAGS=-Wall -Wextra -lncurses

OUT_FILES=out/line.o out/buffer.o out/main.o

all: $(OUTDIR) ute

.PHONY=ute
ute: out/ute

out/ute: $(OUT_FILES)
	$(CC) -g -o out/ute $(OUT_FILES) $(CFLAGS) $(IFLAGS)

out/main.o: src/main.c
	$(CC) -g -c -o out/main.o src/main.c $(CFLAGS) $(IFLAGS)

out/line.o: src/line.c
	$(CC) -g -c -o out/line.o src/line.c $(IFLAGS)

out/buffer.o: src/buffer.c
	$(CC) -g -c -o out/buffer.o src/buffer.c $(IFLAGS)

$(OUTDIR):
	mkdir -p $(OUTDIR)
