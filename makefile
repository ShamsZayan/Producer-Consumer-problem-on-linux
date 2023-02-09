cc = gcc -g
CC = g++ -g

all:command.o

command.o: producer.cpp consumer.cpp
	$(CC)  -o producer producer.cpp
	$(CC)  -o consumer consumer.cpp

