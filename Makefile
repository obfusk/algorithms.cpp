CXXFLAGS  = -Wall -Wextra -Werror -std=c++11 -g -O2
SHELL     = bash

.PHONY: all test clean

all: algorithms

test: all
	diff -Naur algorithms.cpp.out <( ./algorithms < algorithms.cpp.in )

clean:
	rm -fr algorithms
