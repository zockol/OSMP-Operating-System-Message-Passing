
//Hier sind zusätzliche eigene Hilfsfunktionen für die interne Verwendung in der OSMP Bibliothek
//definiert.




#ifndef OSMPlib_h
#define OSMPlib_h

#include "../OSMP.h"

void *isend(OSMP_Request *request);

void *ircv(OSMP_Request *request);

int OSMP_DataSize(OSMP_Datatype datatype);

#endif