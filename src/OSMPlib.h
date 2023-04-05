#ifndef OSMPlib_h
#define OSMPlib_h

int OSMP_Init(int *argc, char ***argv);
int OSMP_Size(int *size);
int OSMP_Rank(int *rank);
int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest);
int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len);
int OSMP_Finalize(void);

#endif