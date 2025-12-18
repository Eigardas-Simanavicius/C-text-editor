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
// init // 
 int main(){
   enableRawMode();
   printf("Welcome to egg, please press :q to exit \n");

   while(1){
   char c = '\0';
   
   // we dont need to do the while loop, due to the vmin and vtime , we it will read automatically/?
   read(STDIN_FILENO, &c,1);

   if(iscntrl(c)){
       printf("%d \r\n",c);
   }else{
       printf("%d ('%c')\r\n", c,c);}

  if(c == CTRL_KEY('q')) break;
   }
   return 0;
}
