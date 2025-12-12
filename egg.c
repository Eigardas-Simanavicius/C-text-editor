#include <stdio.h>
#include <unistd.h>
int main(){
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
