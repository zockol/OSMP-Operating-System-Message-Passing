#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argv, char* argc[]){

    for(int i = 1; i < argv; i++){
        printf("%s ", argc[i]);
        
    }

}


