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

    printf("STRERROR_START: %s, ERRNO == %d\n", strerror(errno), errno);

    pid_t pid, pid2;

    // Kindprozess bei PID = 0
    // Elternprozess da PID = XXXXX und XXXXX ist die PID vom Kind
    int i;

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
            execlp("./output/echoall", "echoall", "Echoall" ,"Test", NULL);

        }
        sleep(3);
    }


    return 0;

}