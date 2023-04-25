
//Hier sind zusätzliche eigene Hilfsfunktionen für die interne Verwendung in der OSMP Bibliothek
//definiert.


#ifndef OSMPlib_h
#define OSMPlib_h
#include "osmplib.c"

int OSMP_Init();
int OSMP_Finalize();
int OSMP_Size();
int OSMP_Rank();
int OSMP_Send();
int OSMP_Recv();
int OSMP_Bcast();


#endif