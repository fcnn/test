all: http

http: main.o http.o
	g++ -o $@ $^

.PHONY: clean
clean:
	rm -f *.o http
