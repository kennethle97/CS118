COMPILER = g++
CFLAGS = -Wall -ggdb -O0
INCL_PATHS =

all: server

server: main.cpp server.cpp server.h
	$(COMPILER) -std=c++20 $(INCL_PATHS) main.cpp server.cpp -o $@ $(CFLAGS)

clean:
	rm -rf server
	rm -rf *.dSYM

