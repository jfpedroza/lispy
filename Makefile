CXX = g++
CC = gcc
CFLAGS = -std=c99 -Wall -c
CXXFLAGS = -std=c++17 -Wall

CSOURCES = mpc.c

CXXSOURCES = parsing.cpp

LIBS = -ledit -lm

default: build

mpc.o:
	$(CC) $(CFLAGS) mpc.c $(LIBS)

myownlisp: mpc.o
	$(CXX) $(CXXFLAGS) parsing.cpp $(LIBS) -o myownlisp 

build: myownlisp
