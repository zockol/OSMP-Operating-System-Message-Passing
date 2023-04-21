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


int contains(pid_t *pid){
    for (int i = 0; i < sizeof(&pid); ++i) {
        for (int j = 0; j < ; ++j) {
            
        }
        
    }
    
}

int main(int argv, char* argc[]) {
    if(argv <=2) {
        pid_t pid[argv];
        int i = 0;
        for( i = 0; i <= argv ;i++){
            pid[i] = fork();
            if(pid != 0){
                printf("Mainprozess %d\n", pid[i]);
            }else{
                printf("Kinderprozess\n");
            }
        }
    }else{
        printf("Es muss die Anzahl der Prozesse angegeben werden");
    }

}