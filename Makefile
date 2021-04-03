## The Makefile for sharer_v1.0.

CC = arm-linux-g++

CFLAGS = -std=c++0x -pthread -W -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD
#CFLAGS = -std=c++11 -pthread -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD

LDFLAGS = 

prefix = 



all:sharer

install:

clean:
	rm -rf ./*.o sharer

sharer:main.o uart.o gpio.o lora.o gnss.o mac.o app.o global.o frame.o message.o nmea0183.o utils.o
	$(CC) $(LFLAGS) -o sharer main.o uart.o gpio.o lora.o gnss.o mac.o app.o global.o frame.o message.o nmea0183.o utils.o

main.o:main.cpp
	$(CC) $(CFLAGS) -c main.cpp

uart.o:uart.cpp
	$(CC) $(CFLAGS) -c uart.cpp

gpio.o:gpio.cpp
	$(CC) $(CFLAGS) -c gpio.cpp

lora.o:lora.cpp
	$(CC) $(CFLAGS) -c lora.cpp

gnss.o:gnss.cpp
	$(CC) $(CFLAGS) -c gnss.cpp

mac.o:mac.cpp
	$(CC) $(CFLAGS) -c mac.cpp

app.o:app.cpp
	$(CC) $(CFLAGS) -c app.cpp

global.o:global.cpp
	$(CC) $(CFLAGS) -c global.cpp

frame.o:frame.cpp
	$(CC) $(CFLAGS) -c frame.cpp

message.o:message.cpp
	$(CC) $(CFLAGS) -c message.cpp

nmea0183.o:nmea0183.cpp
	$(CC) $(CFLAGS) -c nmea0183.cpp

utils.o:utils.cpp
	$(CC) $(CFLAGS) -c utils.cpp

