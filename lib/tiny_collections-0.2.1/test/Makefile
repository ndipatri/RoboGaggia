# Test the tiny-collections library.
# Runs on your local g++, not compiled for the Arduino target.

OUTDIR=../build
OUT=$(OUTDIR)/runtests

CFLAGS=-O2 -Wall -g -std=gnu++17 -I../src/
CC=g++

SOURCES=$(shell ls *.cpp)
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

all: test

help:
	@echo "Run `make test` to compile the test script and run it."
	@echo "Run `make clean` to remove test script build artifacts."

test: clean $(OUT)
	@echo "Running test script..."
	$(OUT)

clean:
	-rm -f *.o
	-rm -f $(OUT)

$(OUT): $(OBJECTS)
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(OUT)

%.o : %.cpp
	$(CC) -x c++ -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY: all clean test help
