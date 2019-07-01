CXX = g++
CC = gcc
CFLAGS = -std=c99 -Wall -c
CXXFLAGS = -std=c++17 -Wall -g

CSOURCES = mpc.c

CXXSOURCES = parsing.cpp

LIBS = -ledit -lm

default: run

mpc.o:
	$(CC) $(CFLAGS) mpc.c $(LIBS)

myownlisp: mpc.o parsing.cpp parsing.h
	$(CXX) $(CXXFLAGS) mpc.o parsing.cpp $(LIBS) -o myownlisp 

build: myownlisp

run: build
	./myownlisp
