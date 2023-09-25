#include <stdio.h>
#include <ncurses.h>

struct {
    int cx, cy;

    int screen_width, screen_height;
} ute = {0};

int main(int argc, char **argv) {
    initscr();
    keypad(stdscr, 1);
    noecho();
    getmaxyx(stdscr, ute.screen_height, ute.screen_width);

    int ch;
    move(ute.cy, ute.cx);
    while (1) {
        ch = getch();

        switch (ch) {
            case KEY_DOWN:
                if(ute.cy < ute.screen_height) {
                    ute.cy++;
                }
                break;
            case KEY_UP:
                if(ute.cy > 0) {
                    ute.cy--;
                }
                break;
            case KEY_RIGHT:
                if(ute.cx < ute.screen_width) {
                    ute.cx++;
                }
                break;
            case KEY_LEFT:
                if(ute.cx > 0) {
                    ute.cx--;
                }
                break;
        }
        move(ute.cy, ute.cx);
    }
    endwin();
    return 0;
}
