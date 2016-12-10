all:
	gcc -ggdb -Wall -o sprat src/*.c src/*.h
clean:
	rm sprat
