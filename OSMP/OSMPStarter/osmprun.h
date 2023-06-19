//
// Created by ubuntu on 26.04.23.
//

#ifndef GRP26_OSMPRUN_H
#define GRP26_OSMPRUN_H



#include "../OSMP.h"

int evaluateArgs(int argc, char *argv[]);
int shm_init(int pidAmount);
int start_shm(int pidAmount);


#endif //GRP26_OSMPRUN_H
