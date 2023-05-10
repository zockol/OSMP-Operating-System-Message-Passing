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

    if(rank == 0){
        OSMP_Bcast();
        
    }else{
        OSMP_Recv();
    }

    return OSMP_SUCCESS;
}