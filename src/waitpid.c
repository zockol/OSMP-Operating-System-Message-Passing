#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>


#define CHILD 0

int main(){

  
    pid_t pid = fork();
    if(!errno){
        printf("ERROR: %s\nProgramm has stopped!", strerror(errno));
    }
    int status;
   
    if (pid != CHILD){
            printf("Parent-Process: PID of child = %d, Iteration: %d \n", pid, 0); 
            waitpid(pid, &status, 0);
            printf("Childprocess finished\n");
    }else{
             for (int j = 0; j < 10; j++) {
                 printf("Child-Process: Iteration: %d \n", j);

             }
        }
       

  
  return 0;

}