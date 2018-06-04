
#all: libdsl.so dsltest
all: dsl so

CXXFLAGS=-fPIC

#libdsl.so: dsl.o
#	$(CXX) -shared -Wl,-soname,libdsl.so -o $@ $^

dsl: main.o
	$(CC) -g -o $@ $^ -ldl -lrt

so:
	mkdir so
.PHONY: clean

clean:
	rm -rf lib* *.o _* dsl so

