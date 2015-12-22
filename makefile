CC=clang++
CXXFLAGS = -std=c++14  -g `llvm-config-3.6 --cxxflags --ldflags --system-libs --libs` -fexceptions

all : biuc

biuc : biuc.o parser.o typechecker.o codegen.o
	$(CC) $^ $(CXXFLAGS) -o $@

biuc.o : biuc.cpp 
	$(CC) -c biuc.cpp $(CXXFLAGS) -o $@

typechecker.o : typechecker.cpp typechecker.h
	$(CC) -c typechecker.cpp $(CXXFLAGS) -o $@

parser.o : parser.cpp parser.h
	$(CC) -c parser.cpp $(CXXFLAGS) -o $@

codegen.o : codegen.cpp codegen.h
	$(CC) -c codegen.cpp $(CXXFLAGS) -o $@

clean : 
	rm -f biuc *.o
