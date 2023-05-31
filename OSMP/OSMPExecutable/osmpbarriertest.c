//
// Created by ubuntu on 25.04.23.
//

#include "../OSMPLib/osmplib.h"

int main(int argc, char *argv[])
{
    int rv = 0, size = 0, rank = 0;
    rv = OSMP_Init( &argc, &argv );
    rv = OSMP_Size( &size );
    rv = OSMP_Rank( &rank );
    printf("before\n");


    OSMP_Barrier();


    printf("after \n");


    return OSMP_SUCCESS;
}