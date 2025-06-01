CXX = ${CXX:-g++}
CXXFLAGS = '--std=c++20'

all: executable clean

%.o: %.cpp
	@$(CXX) -c -g -o $@ $< $(CXXFLAGS)

executable: ShannonEntropy.o
	@$(CXX) $(CXXFLAGS) -o ShannonEntropy ShannonEntropy.o

.PHONY: clean

clean:
	@rm -f *.o
