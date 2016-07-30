
all: libdsl.so.1.0.1 dsltest

CXXFLAGS=-fPIC

libdsl.so.1.0.1: dsl.o
	g++ -shared -Wl,-soname,libdsl.so.1 -o $@ $^

dsltest: dsltest.o
	gcc -g -o $@ $^ -ldl -lrt -ldsl -L.

.PHONY: clean

clean:
	rm -rf lib* *.o dsltest


