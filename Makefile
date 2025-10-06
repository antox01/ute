CC=gcc

OUTDIR=out
IFLAGS=-I./include
CFLAGS=-Wall -Wextra

LD_FLAGS=-lncurses
# OUT_FILES+=out/line.o
OUT_FILES+=out/buffer.o
OUT_FILES+=out/main.o
OUT_FILES+=out/lexer.o
OUT_FILES+=out/utils.o

DEBUG=y
DBG_FLAGS-$(DEBUG)+=-fsanitize=address -g

all: $(OUTDIR) ute

.PHONY=ute
ute: out/ute

out/ute: $(OUT_FILES)
	$(CC) $(DBG_FLAGS-y) -o out/ute $(OUT_FILES) $(LD_FLAGS)

out/main.o: src/main.c include/utils.h include/buffer.h include/lexer.h
	$(CC) $(DBG_FLAGS-y) -c -o out/main.o src/main.c $(CFLAGS) $(IFLAGS)

out/line.o: src/line.c
	$(CC) $(DBG_FLAGS-y) -c -o out/line.o src/line.c $(CFLAGS) $(IFLAGS)

out/buffer.o: src/buffer.c include/buffer.h include/line.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/buffer.o src/buffer.c $(CFLAGS) $(IFLAGS)

out/lexer.o: src/lexer.c include/lexer.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/lexer.o src/lexer.c $(CFLAGS) $(IFLAGS)

out/utils.o: src/utils.c include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/utils.o src/utils.c $(CFLAGS) $(IFLAGS)

$(OUTDIR):
	mkdir -p $(OUTDIR)
