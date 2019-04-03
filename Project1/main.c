#include "constants.h"
#include "transmitter.h"
#include "receiver.h"
#include "utilities.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{	
	if ( (argc != 3) || ((strcmp("/dev/ttyS0", argv[1])!=0) 
		&& (strcmp("/dev/ttyS1", argv[1])!=0) && (strcmp("/dev/ttyS2", argv[1])!=0))) 
	{
	  printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	  exit(1);
	}
	

	if (strcmp(argv[2], "transmitter") == 0)
	{
	  char filename[100], *newLine;
	  
	  printf("Filename: ");
	
	  fgets(filename, 100, stdin);
	  
	  if ((newLine = strchr(filename, '\n')) != NULL) 
      *newLine = '\0';
    	  
		return sendFile(filename, argv[1]);

		// sendFile("heli.jpg", "file2");
	}
	else if (strcmp(argv[2], "receiver") == 0)
	{
		return receiveFile(argv[1]);

		// receiveFile("file2");
	}
	else
	{
		printf("Must specify \"transmitter\" or \"receiver\" as second argument\n");
		return -1;
	}
	
	return 0;
}
