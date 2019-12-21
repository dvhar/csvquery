all: cql

FLAGS = --std=c++17

cql: main.o parser.o scanner.o utils.o filereader.o treetyping.o codegen.o vmachine.o
	clang++ $(FLAGS) -o cql $^

.cc.o: interpretor.h vmachine.h
	clang++ $(FLAGS) -c $<

clean:
	rm *.o

test: test.o utils.o
	clang++ $(FLAGS) -o run $^
	./run
