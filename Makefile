CXX = g++
CC = gcc
CFLAGS = -std=c99 -Wall -c
CXXFLAGS = -std=c++17 -Wall -g

CSOURCES = mpc.c

CXXSOURCES = parsing.cpp lval_error.cpp

LIBS = -ledit -lm

default: run

mpc.o:
	$(CC) $(CFLAGS) mpc.c $(LIBS)

myownlisp: mpc.o parsing.cpp lval_error.cpp parsing.hpp lval_error.hpp
	$(CXX) $(CXXFLAGS) mpc.o $(CXXSOURCES) $(LIBS) -o myownlisp 

build: myownlisp

run: build
	./myownlisp
