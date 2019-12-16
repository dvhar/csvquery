all: cql

cql: main.o parser.o scanner.o utils.o filereader.o treetyping.o codegen.o vmachine.o
	clang++ -o cql $^

.cc.o: interpretor.h
	clang++ -c $<

clean:
	rm *.o

test: test.o utils.o
	clang++ -o run $^
	./run
