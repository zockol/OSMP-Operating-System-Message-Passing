//
// Created by ubuntu on 25.04.23.
//

#include "../OSMPLib/osmplib.h"

int main(int argc, char *argv[]) {
    int rv = 0, size = 0, rank = 0;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    printf("before %d\n", rank);


    OSMP_Barrier();


    printf("after %d\n", rank);
    sleep(10);

    OSMP_Barrier();

    printf("2.after  %d\n", rank);

    return OSMP_SUCCESS;
}