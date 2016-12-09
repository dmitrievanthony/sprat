all:
	gcc -Wall -o sprat src/*.c src/*.h
clean:
	rm sprat
