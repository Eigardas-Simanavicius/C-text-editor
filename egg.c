#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

struct termios orgAttributes;

void disableRawMode(){
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&orgAttributes);
}
void enableRawMode(){

  tcgetattr(STDIN_FILENO,&orgAttributes);
  atexit(disableRawMode);
  // creates a structure to read the current attributes of the terminal
  struct termios terminalAttributes = orgAttributes;
   // turn off ECHO
  terminalAttributes.c_lflag &= ~(ECHO);
  // we pass the new attributes back to the terminal 
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminalAttributes);

}
int main(){
  enableRawMode();
  char c;
  printf("Welcome to egg, please press :q to exit \n");
  while((read(STDIN_FILENO, &c,1) == 1) && quit != 1){
  if(c == ':'){
      read(STDIN_FILENO,&c,1);
      if(c == 'q'){    
        printf("exiting program \n");
        exit(0);
      }
      if(c == 'h'){
        printf("Helping helping helping \n");
      }
    }
  }
  return 0;
}
