#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BAUDRATE B38400 /* bit rate*/
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BUFFER 255
#define FLAG 0x7E
#define ADDR 0x03
#define SET_C 0x03
#define DISC_C 0x0B
#define UA_C 0x07
#define RR_C 0x06
#define REJ_C 0x01
#define ESCAPE 0x7d
#define TRANSMITTER 0
#define RECEIVER 1
#define MAX_ATTEMPTS 3
#define TIMEOUT 3
#define DATASIZE 128 //Max frame size
#define MAX_FILE_SIZE 100000 //100 KB


#endif
