#ifndef HEADER_H_
#define HEADER_H_
#include <stdio.h>
void clearScreen();
void editorAppendRow(char *s, size_t len);
void insertNewRow(int at);
void displayConsole(char cntrl);
void saveToFile(char *filneame);
#endif
