all: bbo sbo bsf asm lg2 run

CC=clang
CFLAGS=-mavx2 -Wall

bbo: bb.c
	$(CC) $(CFLAGS) -D__BBO -o $@ $< -lrt
sbo: bb.c
	$(CC) $(CFLAGS) -D__SBO -o $@ $< -lrt
bsf: bb.c
	$(CC) $(CFLAGS) -D__BSF -o $@ $< -lrt
asm: bb.c
	$(CC) $(CFLAGS) -D__ASM -o $@ $< -lrt
lg2: bb.c
	$(CC) $(CFLAGS) -D__LG2 -o $@ $< -lrt -lm

run:
	@echo bbo:: `./bbo`
	@echo sbo:: `./sbo`
	@echo bsr:: `./asm`
	@echo bsf:: `./bsf`
	@echo lg2:: `./lg2`
clean:
	rm -f bbo sbo bsf asm lg2
