
all: libdsl.so dsltest

CXXFLAGS=-fPIC

libdsl.so: dsl.o
	g++ -shared -Wl,-soname,libdsl.so -o $@ $^

dsltest: dsltest.o
	gcc -g -o $@ $^ -ldl -lrt

.PHONY: clean

clean:
	rm -rf lib* *.o dsltest


