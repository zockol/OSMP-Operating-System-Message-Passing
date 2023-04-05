#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>


#define CHILD 0

int main(){

  
    pid_t pid = fork();
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