// We need a dynamically changing string structure, this will be my appendBuffer
//
//
//
#include <stdlib.h>
#include <string.h>
#define ABUF_INIT {NULL, 0}

struct abuf {
  // pointer to the end
  char *curr;
  int len;
};

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->curr, ab->len + len);

  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->curr = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->curr); }
