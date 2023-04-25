// simple ecoall programm

#include <stdio.h>

int main(int argv, char* argc[]){

    for(int i = 1; i < argv; i++){
        printf("%s ", argc[i]);

    }
    printf("\n");

    return 0;
}


