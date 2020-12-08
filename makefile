CC = gcc
# CFLAGS = -Wall -g
LDLIBS = -lm


bench: bench.o bf.o murmur2.o
	gcc -o bench bench.o bf.o murmur2.o 

bench.o: bench.c 
	gcc -c bench.c -o bench.o

bf.o: bloomfilter.c 
	gcc -c bloomfilter.c -o bf.o

murmur2.o: murmurhash2.c
	gcc -c murmurhash2.c -o murmur2.o

run: bench
	./bench

clean:
	rm -f *.o bench


