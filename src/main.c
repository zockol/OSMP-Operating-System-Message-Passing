#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "echoall.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define CHILD 0

int main(int argv, char* argc[]){

if(argc[1] == 1 || argc[1] == 0){

  pid_t pid = fork();

  if (pid != CHILD){
      printf("PID = %d", pid);
  }
  else if(pid == CHILD){
    printf("");
  }
}
if(argc[1] == 2 || argc[1] == 0){

//Programm, welches ein neues Programm ausführt 

for(int i = 0; i<5;i++)
{
char *programm1[]={"./../output/echoall.o", "%d", i};
        execvp(programm1[0], programm1);
}
}
}