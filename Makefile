all:
	clear
	bison --yacc --defines --output=parser.cpp parser.y
	flex --outfile=al.cpp al.l
	g++ -g -o parser al.cpp parser.cpp
	g++ -g -o avm avm.cpp

clean:
	clear
	rm -rf parser
	rm -rf avm
	rm -rf al.cpp
	rm -rf parser.cpp
	rm -rf parser.hpp
	rm -rf BinaryCode.abc
