#include "transmitter.h"

#include "constants.h"
#include "utilities.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

struct timeval writeTime, readTime;

void sigalrm_handler(int signal)
{
	if (ll.numTransmissions > 0)
	{
		printf("Message timed out! New attempt\n");
		ll.numTransmissions--;		
	}
	else
	{
		printf("Maximum number of attempts reached - Exiting\n");
		exit(1);
	}

	printf("Message timed out!\n");
}

int stateMachine(char* device, char* buffer, int size, char* filename)
{
	al.status = 0;
	al.flag = TRANSMITTER;
	al.dataPacketIndex = 0;

	ll.sequenceNumber = 0;
	ll.numTransmissions = MAX_ATTEMPTS;

	int packageSize = 0, numBytes;

	char** packageArray = calloc((size/DATASIZE + 1) + 2, sizeof(char*));

	while (1)
	{
		if (al.status == 0) // Closed
		{
			al.fileDescriptor = openPort(device, al.flag);
			
			// al.fileDescriptor = open(device, O_WRONLY | O_TRUNC | O_CREAT, 0777);

			if (al.fileDescriptor > 0)
			{
				al.status = 1;
			}
		}
		else if (al.status == 1) // Transfering
		{
			if (packageArray[al.dataPacketIndex] == NULL)
			{
				packageSize = 0;

				packageArray[al.dataPacketIndex] = malloc(DATASIZE + 4 + 1);

				if (al.dataPacketIndex == 0) // Start
				{
					packageArray[al.dataPacketIndex][1 + packageSize++] = 2; // C (2 - start)

					packageArray[al.dataPacketIndex][1 + packageSize++] = 0; // field type (file size)
					packageArray[al.dataPacketIndex][1 + packageSize++] = sizeof(int); // Number of bytes of field
					memcpy(&packageArray[al.dataPacketIndex][1 + packageSize], &size, sizeof(int));
					packageSize += sizeof(int);

					packageArray[al.dataPacketIndex][1 + packageSize++] = 1; // field type (file size)
					packageArray[al.dataPacketIndex][1 + packageSize++] = strlen(filename)+1; // Number of bytes of field
					memcpy(&packageArray[al.dataPacketIndex][1 + packageSize], filename, strlen(filename) + 1);
					packageSize += strlen(filename) + 1;
				}
				else if (al.dataPacketIndex == (size/DATASIZE + 1 + 1)) // End
				{
					packageArray[al.dataPacketIndex][1 + packageSize++] = 3; // C (3 - end)

					packageArray[al.dataPacketIndex][1 + packageSize++] = 0; // field type (file size)
					packageArray[al.dataPacketIndex][1 + packageSize++] = sizeof(int); // Number of bytes of field
					memcpy(&packageArray[al.dataPacketIndex][1 + packageSize], &size, sizeof(int));
					packageSize += sizeof(int);

					packageArray[al.dataPacketIndex][1 + packageSize++] = 1; // field type (file size)
					packageArray[al.dataPacketIndex][1 + packageSize++] = strlen(filename)+1; // Number of bytes of field
					memcpy(&packageArray[al.dataPacketIndex][1 + packageSize], filename, strlen(filename) + 1);
					packageSize += strlen(filename) + 1;
				}
				else // Data Packages
				{
					numBytes = (size  - DATASIZE*(al.dataPacketIndex-1));

					printf("numBytes = %i\n", numBytes);
					
					if (numBytes > DATASIZE)
						numBytes = DATASIZE;

					packageArray[al.dataPacketIndex][1 + packageSize++] = 1; // C (1 - data)
					packageArray[al.dataPacketIndex][1 + packageSize++] = (al.dataPacketIndex) % 255; // Sequence number
					packageArray[al.dataPacketIndex][1 + packageSize++] = numBytes / 256; // The 8 most significant bits in the packageSize.
					packageArray[al.dataPacketIndex][1 + packageSize++] = numBytes % 256;

					memcpy(&packageArray[al.dataPacketIndex][1 + packageSize], buffer+(al.dataPacketIndex-1)*DATASIZE, numBytes);

					packageSize += numBytes;
				}

				packageArray[al.dataPacketIndex][0] = packageSize;
			}

			if (llwrite(al.fileDescriptor, packageArray[al.dataPacketIndex]+1, (unsigned char)packageArray[al.dataPacketIndex][0]) < 0)
				return -1;

			char received[5];

			alarm(TIMEOUT);
			
			int bytes = read(al.fileDescriptor, received, 5);

			alarm(0);
			
			if (bytes > 0)
			{
				ll.numTransmissions = MAX_ATTEMPTS;

				if (gettimeofday(&readTime, NULL) != 0)
					printf("Error getting time!\n");

				unsigned char control = messageCheck(received);
				unsigned char sequenceNumber = (control & 0x80) >> 7; 

				control = 0x0F & control;

				if(control == RR_C)
				{
					if(sequenceNumber == al.dataPacketIndex)
					{
						printf("Sequence Error\n");
					}
					else
					{
						printf("Frame sent sucessfully\n\n");
						al.dataPacketIndex++;
						ll.sequenceNumber = al.dataPacketIndex % 2;	
					}
						
				}
				else if(control == REJ_C)
				{
					if(sequenceNumber == al.dataPacketIndex)
					{
						printf("Corrupt frame sent, sending same frame again!\n\n");
					}
					else
					{
						printf("Frame ahead of receiver \n");
						al.dataPacketIndex--;
						ll.sequenceNumber = al.dataPacketIndex % 2;	
					}
				}
				else
					printf("Unknown message\n");
							
			}

			double deltaTime = (double)(readTime.tv_sec - writeTime.tv_sec) + (double)(readTime.tv_usec - writeTime.tv_usec)/1000/1000; // In seconds 

			if (((float)DATASIZE / deltaTime)/1024 > 0)
				printf("Transfer rate : %.1f KB/s\n", ((float)DATASIZE / deltaTime)/1024);


			if (al.dataPacketIndex > (size/DATASIZE + 1 + 1))
			{
				al.status = 2;
			}
		}
		else if (al.status == 2) // Closing
		{
			// Frees buffers from file transfer
			int j;
			for (j = 0; j < (size/DATASIZE + 1) + 2; j++)
			{
				free(packageArray[j]);
			}

			free(packageArray);
			ll.numTransmissions = MAX_ATTEMPTS;

     	 	int error;

			error = llclose(al.fileDescriptor, TRANSMITTER);

			break;
		}
	}

	return 0;
}

int sendFile(char* filename, char* device)
{	
	struct sigaction sigalrmaction;

	sigalrmaction.sa_handler = sigalrm_handler;
	sigemptyset(&sigalrmaction.sa_mask);
	sigalrmaction.sa_flags = 0;

	if (sigaction(SIGALRM, &sigalrmaction, NULL) < 0)
	{
		fprintf(stderr,"Unable to install SIGALRM handler\n");
		return 1;
	}
	

	int fd = open(filename, O_RDONLY);

	struct stat st;

	if (stat(filename, &st) != 0)
	{
		printf("Missing file!\n");
		return 1;
	}

	int size = st.st_size;

	printf("Size = %i\n", size);
	
	char* buffer = malloc(size);

	int i, bufferSize = 1024, numBytes = bufferSize;

	for (i = 0; numBytes == bufferSize; i++)
	{
		numBytes = read(fd, buffer+i*bufferSize, bufferSize);

		if (numBytes < 0)
		{
			printf("Error reading file!\n");
			return 1;
		}
	}
	close(fd);

	int ret = stateMachine(device, buffer, size, filename);

	free(buffer);

	return ret;
}


int llwrite(int fd, char * buffer, int length)
{
	char package[6 + 2*length];
	int i, j, packageSize = 6+length;

	package[0] = FLAG;
	package[1] = ADDR;
	package[2] = ll.sequenceNumber << 6;
	printf("al.dataPacketIndex = %u\n", al.dataPacketIndex);

	package[3] = package[1] ^ package[2];
	
	int packageLength = length;
	
	for (i = 0; i < length; i++) // Measures size of data
	{
		if (buffer[i] == FLAG || buffer[i] == ESCAPE)
			packageLength++;
	}
	
	for (i = 0; i < length; i++) // Transfers data from buffer to package and  calculates BCC2
	{
		package[4+i] = buffer[i];
		
		if (i == 0)
			package[packageSize-2] = buffer[i];
		else
			package[packageSize-2] ^= buffer[i];
	}

	for (i = 4; i < packageSize - 1; i++) // Stuffing
	{
		if (package[i] == FLAG)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5e;
			i++;
		}
		else if (package[i] == ESCAPE)
		{
			shiftRight(package, packageSize+1, i+1, 1);
			packageSize++;

			package[i] = ESCAPE;
			package[i+1] = 0x5d;
			i++;
		}

	}
	
	package[packageSize-1] = FLAG;

	// printArray(package, packageSize);

	if (gettimeofday(&writeTime, NULL) != 0)
		printf("Error getting time!\n");

	int written = write(fd, package, packageSize);

	if (written < 0)
	{
		printf("Error in transmission\n");
		return -1;
	}
	
	// printf("Frame sent with %i bytes!\n", written);

	return written;
}
