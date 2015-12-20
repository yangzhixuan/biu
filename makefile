CXXFLAGS = -std=c++14 -g `llvm-config --cxxflags --ldflags --system-libs --libs core`

all : biuc

biuc : biuc.o parser.o typechecker.o codegen.o
	g++ $(CXXFLAGS) $^ -o $@

biuc.o : biuc.cpp 
	g++ $(CXXFLAGS) -c biuc.cpp -o $@

typechecker.o : typechecker.cpp typechecker.h
	g++ $(CXXFLAGS) -c typechecker.cpp -o $@

parser.o : parser.cpp parser.h
	g++ $(CXXFLAGS) -c parser.cpp -o $@

codegen.o : codegen.cpp codegen.h
	g++ $(CXXFLAGS) -c codegen.cpp -o $@

clean : 
	rm -f biuc *.o
