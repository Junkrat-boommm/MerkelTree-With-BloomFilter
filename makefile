CC = gcc
# CFLAGS = -Wall -g


bench: bench.o bf.o murmur2.o merkletree.o
	gcc -g -o bench bench.o bf.o murmur2.o merkletree.o -lm

bench.o: bench.c 
	gcc -c -g bench.c -o bench.o

bf.o: bloomfilter.c 
	gcc -c -g bloomfilter.c -o bf.o

murmur2.o: murmurhash2.c
	gcc -c -g murmurhash2.c -o murmur2.o

merkletree.o: merkletree.c
	gcc -c -g merkletree.c -o merkletree.o

clean:
	rm -f *.o bench


