CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude
SRC = src/data_engine.cpp src/upload.cpp
OBJ = $(SRC:.cpp=.o)

all: bin/upload bin/findrec

bin/upload: src/data_engine.o src/upload.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/findrec: src/data_engine.o src/findrec.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf bin src/*.o data/*.db
