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


int main(int argv, char* argc[]) {
if(argv <=2){
    pid_t pid[(int)argc[1]];
    int i = 0;
    for( i = 0; i < argc[1];i++){
    pid[i] = fork();

    if(pid[i] != 0){
        printf("Mainprozess %d", pid[i]);
    }else{
        printf("Kinderprozess");
    }

    }
}else{
    printf("Es muss die Anzahl der Prozesse angegeben werden");
}

}