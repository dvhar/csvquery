all: cql

cql: main.o parser.o scanner.o utils.o filereader.o
	clang++ -o cql $^

.cc.o: interpretor.h
	clang++ -c $<

clean:
	rm *.o

test: filereader.o test.o utils.o
	clang++ -o run $^
