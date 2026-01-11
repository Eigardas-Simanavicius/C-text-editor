#ifndef APPENDBUFFER_H_
#define APPENDBUFFER_H_

#define ABUF_INIT {NULL, 0}
struct abuf {
  char *curr;
  int len;
};
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

#endif
