#include "header.h"
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// defines //
#define CTRL_KEY(k) ((k) & 0x1f)
struct editorConfig {
  int rows;
  int cols;
  struct termios orgAttributes;
};

struct editorConfig editor;
void errorPrint(const char *s) {
  // spits an error back at us when something goes wrong
  clearScreen();
  perror(s);
  exit(1);
}
// terminal slop, stuff we need to change the termnial //
void disableRawMode() {
  // setting terminal back to standard.
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.orgAttributes) == -1)
    errorPrint("tcset");
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &editor.orgAttributes);
  atexit(disableRawMode);

  // creates a structure to read the current attributes of the terminal
  struct termios terminalAttributes = editor.orgAttributes;
  // turn off ECHO
  terminalAttributes.c_iflag &= ~(IXON);
  terminalAttributes.c_oflag &= ~(OPOST);
  terminalAttributes.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  // vmin the amount of bytes we wait before return read, and vtime how often
  terminalAttributes.c_cc[VMIN] = 0;
  terminalAttributes.c_cc[VTIME] = 1;
  // we pass the new attributes back to the terminal
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminalAttributes);
}

char readKey() {
  int nread;
  char c;
  // EAGAIN: "Resoursce temporerily unavailabe."
  if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
    errorPrint("reading error");
  }
  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
  }
}

//*** input ***/*/
void processKey() {
  char c = readKey();
  switch (c) {
  case CTRL_KEY('q'):
    clearScreen();
    exit(0);
    break;
  }
}

//** output **//
//
void editorDrawRows() {
  int y;
  for (y = 0; y < editor.rows; y++) {
    write(STDOUT_FILENO, "~", 1);
    if (y < editor.rows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void clearScreen() {
  // we are writing 4,ytes to the file, x1b is the escape character, and [2J is
  // te other 3 bytes
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}
// init //
void initEditor() {
  if (getWindowSize(&editor.rows, &editor.cols) == -1)
    errorPrint("windowsize Fail");
}
int main() {
  enableRawMode();
  printf("Welcome to egg, please press :q to exit \n");
  initEditor();

  while (1) {
    clearScreen();
    processKey();
  }

  return 0;
}
