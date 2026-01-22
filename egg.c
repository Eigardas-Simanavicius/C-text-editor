#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "appendBuffer.h"
#include "header.h"
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

// defines //
#define CTRL_KEY(k) ((k) & 0x1f)

enum moveKeys {
  ARROW_LEFT = 1000,
  ARROW_RIGHT = 1001,
  ARROW_UP = 1002,
  ARROW_DOWN = 1003,
  PAGE_UP = 1004,
  PAGE_DOWN = 1005

};

typedef struct erow {
  int size;
  char *chars;
} erow;

struct editorConfig {
  int cx, cy;
  int rows; // screenrows
  int cols; // windowscolumsn
  int usedrows;
  erow *erow;
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

int readKey() {
  char c;
  // EAGAIN: "Resoursce temporerily unavailabe."

  if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
    errorPrint("reading error");
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDOUT_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDOUT_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDOUT_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '5':
            return PAGE_UP;
            break;
          case '6':
            return PAGE_DOWN;
            break;
          }
        }
      }
      switch (seq[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
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
  // char *lines = editor.erow->chars;
  int c = readKey();
  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;

  case ARROW_UP:
    if (editor.cy != 0) {
      editor.cy--;
    }
    break;
  case ARROW_DOWN:
    if (editor.cy != editor.rows - 1) {
      editor.cy++;
    }
    break;
  case ARROW_LEFT:
    if (editor.cx != 0) {
      editor.cx--;
    }
    break;
  case ARROW_RIGHT:
    if (editor.cx != editor.cols - 1) {
      editor.cx++;
    }
    break;
  case PAGE_UP:
    editor.cy = 0;
    break;
  case PAGE_DOWN:
    editor.cy = editor.rows - 1;
    break;
  }
}

//** output **//
//
void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < editor.rows; y++) {
    abAppend(ab, "~ ", 1);
    if (y < editor.usedrows) {
      abAppend(ab, editor.erow[y].chars, strlen(editor.erow[y].chars));
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < editor.rows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void clearScreen() {
  struct abuf ab = ABUF_INIT;
  // we are writing 4,ytes to the file, x1b is the escape character, and [2J
  // is te other 3 bytes
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", editor.cy + 1, editor.cx + 2);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.curr, ab.len);
  abFree(&ab);
}
void editorAppendRow(char *s, size_t len) {
  editor.erow =
      realloc(editor.erow, sizeof(editor.erow) * (editor.usedrows + 1));

  int curr = editor.usedrows;
  editor.erow[curr].size = len;
  editor.erow[curr].chars = malloc(len + 1);
  memcpy(editor.erow[curr].chars, s, len);
  editor.erow[curr].chars[len] = '\0';
  editor.usedrows++;
}
// file io//
void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    errorPrint("file not found");

  char line[100];

  int linecap = 0;
  ssize_t linelen;
  while (fgets(line, 100, fp)) {
    linelen = strlen(line);
    printf("%s", line);

    if (linelen != -1) {
      while (linelen > 0 &&
             (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
        linelen--;
      }
      editorAppendRow(line, linelen);
    }
  }
  fclose(fp);
}

// init //
void initEditor() {
  editor.cx = 0;
  editor.cy = 0;
  editor.usedrows = 0;
  editor.erow = NULL;
  if (getWindowSize(&editor.rows, &editor.cols) == -1)
    errorPrint("windowsize Fail");
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();

  if (argc >= 2) {

    editorOpen(argv[1]);
  }

  while (1) {
    clearScreen();
    processKey();
  }

  return 0;
}
