CXXFLAGS = -std=c++14 -g `llvm-config --cxxflags --ldflags --system-libs --libs`

all : biuc

biuc : biuc.o parser.o typechecker.o codegen.o
	g++ $^ $(CXXFLAGS) -o $@

biuc.o : biuc.cpp 
	g++ -c biuc.cpp $(CXXFLAGS) -o $@

typechecker.o : typechecker.cpp typechecker.h
	g++ -c typechecker.cpp $(CXXFLAGS) -o $@

parser.o : parser.cpp parser.h
	g++ -c parser.cpp $(CXXFLAGS) -o $@

codegen.o : codegen.cpp codegen.h
	g++ -c codegen.cpp $(CXXFLAGS) -o $@

clean : 
	rm -f biuc *.o *.ll *.s
