CXXFLAGS = -std=c++14

all : biuc

biuc : biuc.o parser.o typechecker.o
	g++ $(CXXFLAGS) $^ -o $@

biuc.o : biuc.cpp 
	g++ $(CXXFLAGS) -c biuc.cpp -o $@

typechecker.o : typechecker.cpp typechecker.h
	g++ $(CXXFLAGS) -c typechecker.cpp -o $@

parser.o : parser.cpp parser.h
	g++ $(CXXFLAGS) -c parser.cpp -o $@

clean : 
	rm -f biuc *.o
