CC = g++ -std=c++17 -Wno-sign-compare
DBGFLAGS = -g -Wall -o
OPTFLAGS = -O3 -DNDEBUG -o	

all: opt discal

src/%.o: src/%.cpp src/headers.h
	$(CC) -c $< $(OPTFLAGS) $@

opt: src/main.o src/func.o
	$(CC) $^ $(OPTFLAGS) $@ 

discal: src/displacement.o src/func.o
	$(CC) $^ $(OPTFLAGS) $@ 

.PHONY: clean
clean:
	$(RM) opt discal src/*.o


# Notes:
#   -c for linking source files, not for the final binary
#   do NOT include .h files in the second line when building .o files
#   % means arbitrary 
#   $^ means all dependencies
#   $< means the first dependency
#   $@ means the target file
