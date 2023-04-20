#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>


// Authors: Felix Grüning, Patrick Zockol
// Last Change: 05.04.2023
// Task: 1. Praktikumstermin | UNIX Prozessmanagement


#define CHILD 1

int main(int argv, char* argc[]){
    int argProcess = argc[1]
    pid_t pid[argProcess];

    // Kindprozess bei PID = 0
    // Elternprozess da PID = XXXXX und XXXXX ist die PID vom Kind
    for(int i = 0; i< pid; i++)){
        pid[i] = fork();
        if(pid[i]== 0){

        }
}
    pid = fork();
    if (pid != 0) {
        pid2 = fork();

    }

    for (i = 0; i < 3; i++) {

        if (pid > 0 && pid2 > 0) {
            printf("Elternprozess PID = %d\n", pid);
            waitpid(pid, NULL, WNOHANG);
            waitpid(pid2, NULL, WNOHANG);
        } else {
            printf("Kindprozess\n");


        }
        sleep(3);
    }


    return 0;

}