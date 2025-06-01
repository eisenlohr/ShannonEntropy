CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3

all: executable clean

%.o: %.cpp
	@$(CXX) -c -o $@ $< $(CXXFLAGS)

executable: ShannonEntropy.o
	@$(CXX) $(CXXFLAGS) -o ShannonEntropy ShannonEntropy.o

.PHONY: clean

clean:
	@rm -f *.o
