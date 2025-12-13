#include <stdio.h>
#include <unistd.h>
#include <termios.h>
void enableRawMode(){
  // creates a structure to read the current attributes of the terminal
  struct termios terminalAttributes;
  // this gets the attributes of the terminal and puts them in the struct
  tcgetattr(STDIN_FILENO,&terminalAttributes);
  // turn off ECHO
  terminalAttributes.c_lflag &= ~(ECHO);
  // we pass the new attributes back to the terminal 
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminalAttric butes);

}
int main(){
  enableRawMode();
  char c;
  char checkexit[2];
  int quit = 0;
  printf("Welcome to egg, please press :q to exit \n");
  while((read(STDIN_FILENO, &c,1) == 1) && quit != 1){
  if(c == ':'){
           fgets(checkexit  ,sizeof(checkexit),stdin);
      if(checkexit[0] == 'q'){    
        printf("exiting program \n");
        quit = 1;
      }
    }
  }
  return 0;
}
