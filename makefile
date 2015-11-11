CXXFLAGS = -std=c++14

all : parser

parser : parser.o 
	g++ $(CXXFLAGS) $? -o $@

parser.o : parser.cpp parser.h
	g++ $(CXXFLAGS) -c parser.cpp -o $@

clean : 
	rm -f parser *.o
