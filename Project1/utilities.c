#include "utilities.h"

#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

void swap(char* a, char*b)
{
	char temp = *a;
	*a = *b;
	*b = temp;
}

int abs(int a)
{
	if (a < 0)
		return -a;
	return a;
}

void printPercentage(double percentage)
{
	printf("<");

	int i, length = 15 /* length of the percentage bar */;
	for (i = 0; i < length; i++)
	{
		if ((double)i/length < percentage)
			printf("|");
		else
			printf(" ");
	}

	printf(">%.1f%%\n", percentage*100);
}

void shiftRight(char* buffer, int size, int position, int shift)
{
	int i, j;

	for (j = 0; j < shift; j++)
	{
		size++;
		buffer[size-1] = 0;

		for (i = size-2; i >= position; i--)
		{
			swap(&buffer[i], &buffer[i+1]);
		}

		position++;
	}
}

void shiftLeft(char* buffer, int size, int position, int shift)
{
	int i, j;

	for (j = 0; j < shift; j++)
	{

		for (i = position-1; i < size; i++)
		{
			swap(&buffer[i], &buffer[i+1]);
		}

		size--;
		position--;
	}
}


void printArray(char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		printf("%i\n", arr[i]);
	}
	printf("\n");
}


int messageCheck(char received[])
{
	char control, bcc1, bcc2;
	int i;

	if (received[0] == FLAG && received[1] == ADDR && received[4] == FLAG)
	{
		control = received[2];
		bcc1 = received[3];

		if (bcc1 == received[1] ^ control)
			return control;
	}

	return -1; //Error
}

int openPort(char* device, int flag)
{
	int fd = open(device, O_RDWR | O_NOCTTY);

	if (fd < 0)
	{
		printf("Unable to open serial port\n");
		exit(-1);
	}

	return llopen(fd, flag);
}

int closePort(int fd, int flag)
{
	// llclose(fd);
	// close(fd);
	
	return 1;
}

int llopen(int fd, int flag)
{
	int c, res;

  /*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
  */

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
	  perror("tcgetattr");
	  exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]	= 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]	= 1;   /* blocking read until 1 chars received */



  /*
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	leitura do(s) próximo(s) caracter(es)
  */


	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
	  perror("tcsetattr");
	  exit(-1);
	}

	printf("New termios structure set\n");

	char buf[5];
	int received;

	if (flag == TRANSMITTER)
	{
		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = SET_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if(write(fd, buf, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("SET sent!\n");


		alarm(TIMEOUT);

		received = read(fd, buf, 5);

		alarm(0);

		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}

		int status = messageCheck(buf);

		if (status != UA_C)
		{
			printf("Unknown message\n");
			return -1;
		}
	}
	else if (flag == RECEIVER)
	{
		// alarm(TIMEOUT);

		received = read(fd, buf, 5);

		// alarm(0);

		if(received < 0)
		{
			printf("Error in receiving end\n");
			return -1;
		}

		int status = messageCheck(buf);

		if (status != SET_C)
		{
			printf("Unknown message\n");
			return -1;
		}

		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = UA_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if(write(fd, buf, 5) < 0)
		{
			printf("Error in transmission\n");
			return -1;
		}

		printf("Message sent!\n");
	}

	return fd;
}


int llclose(int fd, int flag)
{
	char buf[5];
	int received;

	// tcflush(fd, TCIFLUSH);

	if (flag == TRANSMITTER)
	{
		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = DISC_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if(write(fd, buf, 5) < 0)
		{
			printf("Error in llclose transmission\n");
			return -1;
		}

		while (1)
		{
			received = read(fd, buf, 5);

			if (received < 0)
			{
				printf("Error in receiving end in llclose\n");
				continue;
			}

			unsigned char status = messageCheck(buf);

			if (status != DISC_C)
			{
				// printf("DISC_C not received, received: %u\n", status);
				continue;
			}

			break;
		}
		

		buf[2] = UA_C;
		buf[3] = buf[1] ^ buf[2];

		if (write(fd, buf, 5) < 0)
		{
			printf("Error in second llclose transmission\n");
			return -1;
		}

	}
	else if (flag == RECEIVER)
	{
		while (1)
		{
			received = read(fd, buf, 5);

			if (received < 0)
			{
				printf("Error in receiving end in llclose\n");
				continue;
			}

			unsigned char status = messageCheck(buf);

			if (status != DISC_C)
			{
				// printf("DISC_C not received, trying again\n");
				continue;
			}

			break;
		}

		//printf("DISC received\n");

		buf[0] = FLAG;
		buf[1] = ADDR;
		buf[2] = DISC_C;
		buf[3] = buf[1] ^ buf[2];
		buf[4] = FLAG;

		if (write(fd, buf, 5) < 0)
		{
			printf("Error in llclose transmission\n");
			return -1;
		}

		//printf("DISC sent by receiver!\n");

		while (1)
		{
			received = read(fd, buf, 5);

			if (received < 0)
			{
				printf("Error in receiving end in llclose\n");
				continue;
			}

			unsigned char status = messageCheck(buf);

			if (status != UA_C)
			{
				// printf("UA_C not received, trying again\n");
				continue;
			}

			break;
		}
	}

	if (tcsetattr(fd,TCSANOW,&oldtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}

 	close(fd);

 	printf("Connection closed sucessfully!\n");

	return 0;
}
