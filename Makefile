## Russell James Makefile
#CS 344, Winter 2021

## variables for filenames
prog1 = smallsh
srcFile1 = smallsh.c
#srcFile2 = Arena.cpp

## variables for object names
obj1 = $(srcFile1:.c=.o)
#obj2 = $(srcFile2:.c=.o)

## variables for header file names
#header2 = $(srcFile2:.c=.hpp)

## variables for compiler and flags
CXX = gcc
CXFlags = -c -std=gnu99 -g -Wall

## valgrind flags
VFlags = --leak-check=full -v

#all: $(projName)

$(prog1): $(obj1) 
	$(CXX) -std=gnu99 -g $(obj1) -o $(prog1)

$(obj1): $(srcFile1)
	$(CXX) $(CXFlags) $(srcFile1)
#
#$(obj2): $(srcFile2) $(header2)
#	$(CXX) $(CXFlags) $(srcFile2)
#

clean:
	rm *.o $(prog1)

valgrind:
	valgrind $(VFlags) ./$(prog1)
