//
// Created by fegrue on 21.04.23.
//
#include <stdio.h>
#include "stdlib.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "osmprun.h"


int contains(pid_t pid[]){
    int var = 0;
    for (int i = 0; i < sizeof(pid)/sizeof(pid[0]); ++i) {
            if(pid[i]!=0){
                var+=1;
            }

    }
    return var;
}

int main(int argv, char* argc[]) {
    if(argv <=2) {
        pid_t pid[argv];
        int i = 0;
        for( i = 0; i <= argv ;i++){
            if(!contains(pid)){
                pid[i] = fork();
                printf("Mainprozess %d\n", pid[i]);
                for(int j = 0; j<(sizeof(pid)/sizeof(pid[0]));j++){
                    waitpid(pid[j]);
                }
            }else{
                printf("Kinderprozess\n");
            }
        }
    }else{
        printf("Es muss die Anzahl der Prozesse angegeben werden");
    }

}