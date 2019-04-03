CC		 = gcc
CFLAGS	 = -g
LDFLAGS  =
DEPS	 = main.c receiver.c receiver.h transmitter.c transmitter.h utilities.c utilities.h constants.h makefile
OBJFILES = receiver.o transmitter.o utilities.o main.o
TARGET	 = main

all: $(TARGET)

$(TARGET): $(OBJFILES) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~