CC = g++
CFLAGS = -O2 -Wall
LFLAGS =
PTHREADFLAGS=-pthread

fscp: main.o sender.o receiver.o ack.o
	$(CC) $(CFLAGS) $(PTHREADFLAGS) -o fscp main.o sender.o receiver.o ack.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

sender.o: sender.cpp
	$(CC) $(CFLAGS) $(PTHREADFLAGS) -c sender.cpp

receiver.o: receiver.cpp
	$(CC) $(CFLAGS) $(PTHREADFLAGS) -c receiver.cpp

ack.o: ack.cpp
	$(CC) $(CFLAGS) -c ack.cpp

clean:
	rm -f *.o