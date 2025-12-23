 #include <stdio.h>
 #include <unistd.h>
 #include <termios.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <ctype.h>
 #include <stdio.h>

 // defines //
 #define CTRL_KEY(k) ((k) & 0x1f)
 struct termios orgAttributes;
void errorPrint(const char *s){
   // spits an error back at us when something goes wrong.
   perror(s);
   exit(1);
 }
// terminal slop, stuff we need to change the termnial //
 void disableRawMode(){
   // setting terminal back to standard.
   if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&orgAttributes) == -1) errorPrint("tcset");

 }

 void enableRawMode(){
   tcgetattr(STDIN_FILENO,&orgAttributes);
   atexit(disableRawMode);
   // creates a structure to read the current attributes of the terminal
   struct termios terminalAttributes = orgAttributes;
    // turn off ECHO
   terminalAttributes.c_iflag &= ~(IXON);
   terminalAttributes.c_oflag &= ~(OPOST);
   terminalAttributes.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
   // vmin the amount of bytes we wait before return read, and vtime how often
   terminalAttributes.c_cc[VMIN] = 0;
   terminalAttributes.c_cc[VTIME] = 1;
   // we pass the new attributes back to the terminal 
   tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminalAttributes);
 }

 char readKey(){
  int nread;
  char c;
  // EAGAIN: "Resoursce temporerily unavailabe."
  if(read(STDIN_FILENO, &c,1) == -1 && errno != EAGAIN){
     errorPrint("reading error");
   }
    return c;
 }

//*** input ***/*/
 void processKey(){
   char c = readKey();
   printf("%c",&c);
   switch(c){
     case CTRL_KEY('q'):
      exit(0);
      break;
   }

   
 }

 //** output **// 
 //

 void clearScreen(){
   // we are writing 4,ytes to the file, x1b is the escape character, and [2J is te other 3 bytes
   //
   write(STDOUT_FILENO,"\x1b[2J",4);
 }
// init // 
 int main(){
   enableRawMode();
   printf("Welcome to egg, please press :q to exit \n");

   while(1){
     clearScreen();
    processKey();
   }

   return 0;
 }

