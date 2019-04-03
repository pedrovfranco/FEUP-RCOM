#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "utilities.h"

linkLayer ll;
applicationLayer al;

int stateMachine(char* device, char* buffer, int size, char* filename);
int llwrite(int fd, char * buffer, int length);
int sendFile(char* filename, char* device);


#endif
