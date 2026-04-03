#include <asm-generic/errno.h>
#include <stddef.h>
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
#define ENTER_KEY 13
#define BACK_SPACE 127
#define TAB 9

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
  int cols; // windowscolumsn//
  int currRow;
  int usedrows;
  int offset;
  int displayon;
  int changed;
  char *filename;
  char *msg;
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
  terminalAttributes.c_iflag &= ~(ICRNL | IXON);
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

void insertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) {
    at = row->size - 1;
    editor.cx = at;
  }

  if (row->chars[0] == ' ' && c != ' ' && at == 0) {
    row->chars[0] = c;
  } else {
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    editor.cx++;
    row->size++;
    row->chars[at] = c;
  }
}

void deleteChar(erow *row, int at) {
  if (at < 0 || at > row->size) {
    at = row->size - 1;
    editor.cx = at;
  }
  if (at > 0) {
    row->chars = realloc(row->chars, row->size - 2);
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at + 1);
    row->size--;

  } else {
    row->chars[0] = ' ';
  }
  if (editor.cx > 1) {
    editor.cx--;
  }
}

void insertNewRow(int at) {
  editor.erow = realloc(editor.erow, sizeof(erow) * (editor.usedrows + 1));
  memmove(&editor.erow[at + 1], &editor.erow[at],
          sizeof(erow) * (editor.usedrows - at));
  editor.erow[at + 1].size = 1;
  editor.erow[at + 1].chars = malloc(1);
  editor.erow[at + 1].chars[0] = '\0';
  editor.usedrows++;
  editor.cx = 0;

  if (editor.cy != editor.rows - 1) {
    if (editor.cy == ((editor.rows / 6) * 5)) {
      editor.offset++;
    } else {
      editor.cy++;
    }
    editor.currRow++;
    if (editor.erow[editor.currRow].size != 0) {
      editor.cx = 0;
    } else {
      editor.cx = editor.erow[editor.currRow].size;
    }
  }
}
void processKey() {
  int c = readKey();
  if (c != CTRL_KEY('q') && c != 0) {
    editor.changed = 1;
  }
  switch (c) {
  case CTRL_KEY('q'):
    if (editor.changed == 1) {
      if (editor.displayon != 1) {
        displayConsole('q');
      } else {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
      }
    } else {
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
    }
    break;
  case CTRL_KEY('c'):
    displayConsole('c');
    break;
  case CTRL_KEY('s'):
    saveToFile(editor.filename);
    displayConsole('s');
    break;
  case ARROW_UP:
    if (editor.cy != 0) {
      if (editor.cy == (editor.rows / 6) && editor.offset != 0) {
        editor.offset--;
      } else {
        editor.cy--;
      }
      editor.currRow--;
    }
    break;
  case ARROW_DOWN:
    if (editor.cy != editor.rows - 1) {
      if (editor.cy == ((editor.rows / 6) * 5)) {
        editor.offset++;
      } else {
        editor.cy++;
      }
      editor.currRow++;
      if (editor.erow[editor.currRow].size != 0) {
        editor.cx = 0;
      } else {
        editor.cx = editor.erow[editor.currRow].size;
      }
    }
    break;
  case ARROW_LEFT:
    if (editor.cx != 0)
      editor.cx--;
    break;
  case ARROW_RIGHT:
    if (editor.cx != editor.cols - 1) {
      editor.cx++;
    }
    if (editor.erow[editor.currRow].size < editor.cx) {
      // insertChar(&editor.erow[editor.currRow], editor.cx, 104);
    }
    break;
  case PAGE_UP:
    if (editor.currRow > editor.rows) {
      editor.offset = editor.offset - editor.rows;
      editor.currRow = editor.currRow - editor.rows;
    }
    break;
  case PAGE_DOWN:
    if (editor.cy <= editor.rows / 6) {
      editor.cy = editor.rows / 6;
      editor.currRow = editor.currRow + editor.rows / 6;
    }
    editor.offset = editor.offset + editor.rows;
    editor.currRow = editor.currRow + editor.rows;
    break;
  case ENTER_KEY:
    insertNewRow(editor.currRow);
    break;
  //  editor.currRow++;
  case BACK_SPACE:
    if (editor.cx > 0) {
      deleteChar(&editor.erow[editor.currRow], editor.cx - 1);
    }
    break;
  case TAB:
    insertChar(&editor.erow[editor.currRow], editor.cx, ' ');
    insertChar(&editor.erow[editor.currRow], editor.cx, ' ');
    break;
  default:
    if (c > 0 && c != 13 && c != 8) {
      insertChar(&editor.erow[editor.currRow], editor.cx, c);
    }
    break;
  }

  while (editor.currRow + 1 > editor.usedrows) {
    editorAppendRow("", 1);
  }
}

//** output **//
void editorDrawRows(struct abuf *ab) {
  int y;
  int i;
  int len;
  int curr = 0;
  char buffer[100];
  char *stringBuf = malloc(editor.cols);
  for (y = 0; y < editor.rows; y++) {
    len = 0;
    char buffer[editor.cols];
    if (editor.displayon == 1) {
      if (y == editor.rows - 2) {
        for (i = 0; i < editor.cols; i++) {
          buffer[i] = '-';
        }
        abAppend(ab, buffer, editor.cols);
      } else if (y == editor.rows - 1) {
        abAppend(ab, editor.msg, strlen(editor.msg));
      }
    }
    if (editor.displayon == 1 &&
        (y == editor.rows - 2 || y == editor.rows - 1)) {

    } else {
      abAppend(ab, "~ ", 3);
    }
    curr = y + editor.offset;
    if (curr < editor.usedrows) {
      int len = strlen(editor.erow[curr].chars);
      abAppend(ab, editor.erow[curr].chars, len);
    }

    abAppend(ab, "\x1b[K", 4);
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
  editor.erow = realloc(editor.erow, sizeof(erow) * (editor.usedrows + 1));
  int curr = editor.usedrows;
  editor.erow[curr].size = len;
  editor.erow[curr].chars = malloc(len + 1);
  memcpy(editor.erow[curr].chars, s, len);
  editor.erow[curr].chars[len] = '\0';
  editor.usedrows++;
}

// file io//
void saveToFile(char *filename) {
  FILE *fp = fopen(filename, "w");
  int i;
  for (i = 0; i < editor.usedrows; i++) {
    fputs(editor.erow[i].chars, fp);
  }
  fclose(fp);
}
void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    errorPrint("file not found");

  char *line;

  size_t linecap = 0;
  ssize_t linelen;

  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
      linelen--;
    }
    editorAppendRow(line, linelen);
  }

  // free(line);
  fclose(fp);
}

// init //
//
void WindowSizeget() {
  if (getWindowSize(&editor.rows, &editor.cols) == -1) {
    errorPrint("windowssizefail");
  }
}

void initEditor() {
  editor.cx = 0;
  editor.cy = 0;
  editor.usedrows = 0;
  editor.currRow = 0;
  editor.offset = 0;
  editor.displayon = -1;
  editor.erow = NULL;
  editor.changed = 0;
  WindowSizeget();
}

void displayConsole(char cntrl) {
  editor.displayon = 1;
  switch (cntrl) {
  case 'q':
    editor.msg =
        "You have unchanged Changes, use cntrl+s to save before using cntrl q, "
        "or just use cntrl q to quit without saving (cntl C to close message) ";
    break;
  case 'c':
    editor.displayon = editor.displayon * -1;
    break;
  case 's':
    editor.msg = "Data save to file Cntrl C to close this console";
    editor.changed = 0;
    break;
  }
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();

  if (argc >= 2) {
    editor.filename = argv[1];
    editorOpen(argv[1]);
  } else {
    editor.filename = NULL;
    editorAppendRow("", 1);
  }

  while (1) {
    WindowSizeget();
    clearScreen();
    processKey();
  }

  return 0;
}
