all: libclassifier.so

libclassifier.so: simd_math.o classifier.o
	gcc -shared -o libclassifier.so simd_math.o classifier.o

simd_math.o: simd_math.asm
	nasm -f elf64 simd_math.asm -o simd_math.o

classifier.o: classifier.c
	gcc -c -fPIC -O3 classifier.c -o classifier.o

clean:
	rm -f *.o *.so