#ifndef RECEIVER_H
#define RECEIVER_H

#include "utilities.h"
#include <sys/time.h>

linkLayer ll;
struct timeval writeTime2, readTime2;

int stateMachineReceiver(applicationLayer *al, char* device, int *fileSize, char *filename);
int receiveFile(char *device);
int llread(int fd, char *buffer);
int destuff(char* buffer, int* size);
char headerCheck(char received[]);
int sendAnswer(int fd, char control);
int readDataPacket(int *fd, applicationLayer *app, char *buffer, char *filename, int *fileSize, int packetSize, int* bytesReceived);
int checkControlDataPacket(int i, char *buffer, char *filename, int *fileSize, int packetSize);
int trailerCheck(char received[], int size);

#endif
