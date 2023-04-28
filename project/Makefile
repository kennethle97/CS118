CC=g++
CCFLAGS=-I. -std=c++20
DEPS=
OBJ=server.o
USERID=804953883

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

all: server
server: $(OBJ)
	$(CC) -o $@ $^ $(CCFLAGS)

clean:
	rm -rf *.o server *.tar.gz

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .