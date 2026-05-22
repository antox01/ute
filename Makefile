CC=gcc

OUTDIR=out
IFLAGS-y=-I./include
CFLAGS=-Wall -Wextra

GUI=y

IFLAGS-$(GUI)+= -I./external $(shell pkg-config --cflags freetype2)

LD_FLAGS-y$(GUI)=-lncurses
LD_FLAGS-$(GUI)=-framework Cocoa -framework OpenGL $(shell pkg-config --libs freetype2)
# OUT_FILES+=out/line.o
OUT_FILES-y+=out/buffer.o
OUT_FILES-y+=out/bindings.o
OUT_FILES-y$(GUI)+=out/main.o
OUT_FILES-$(GUI)+=out/gui.o
OUT_FILES-y+=out/lexer.o
OUT_FILES-y+=out/history.o
OUT_FILES-y+=out/utils.o
OUT_FILES-y+=out/editor.o
OUT_FILES-y+=out/commands.o

DEBUG=y
DBG_FLAGS-$(DEBUG)+=-fsanitize=address -g

all: $(OUTDIR) ute

.PHONY=ute
ute: out/ute

out/ute: $(OUT_FILES-y)
	$(CC) $(DBG_FLAGS-y) -o out/ute $(OUT_FILES-y) $(LD_FLAGS-y)

out/main.o: src/main.c include/utils.h include/buffer.h include/lexer.h include/history.h
	$(CC) $(DBG_FLAGS-y) -c -o out/main.o src/main.c $(CFLAGS) $(IFLAGS-y)

out/gui.o: src/gui.c include/utils.h include/buffer.h include/lexer.h include/history.h
	$(CC) $(DBG_FLAGS-y) -c -o out/gui.o src/gui.c $(CFLAGS) $(IFLAGS-y)

out/line.o: src/line.c
	$(CC) $(DBG_FLAGS-y) -c -o out/line.o src/line.c $(CFLAGS) $(IFLAGS-y)

out/buffer.o: src/buffer.c include/buffer.h include/line.h include/utils.h include/history.h
	$(CC) $(DBG_FLAGS-y) -c -o out/buffer.o src/buffer.c $(CFLAGS) $(IFLAGS-y)

out/bindings.o: src/bindings.c include/bindings.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/bindings.o src/bindings.c $(CFLAGS) $(IFLAGS-y)

out/history.o: include/utils.h include/history.h src/history.c include/buffer.h
	$(CC) $(DBG_FLAGS-y) -c -o out/history.o src/history.c $(CFLAGS) $(IFLAGS-y)

out/lexer.o: src/lexer.c include/lexer.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/lexer.o src/lexer.c $(CFLAGS) $(IFLAGS-y)

out/utils.o: src/utils.c include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/utils.o src/utils.c $(CFLAGS) $(IFLAGS-y)

out/editor.o: src/editor.c include/editor.h include/commands.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/editor.o src/editor.c $(CFLAGS) $(IFLAGS-y)

out/commands.o: src/commands.c include/commands.h include/utils.h
	$(CC) $(DBG_FLAGS-y) -c -o out/commands.o src/commands.c $(CFLAGS) $(IFLAGS-y)

$(OUTDIR):
	mkdir -p $(OUTDIR)
