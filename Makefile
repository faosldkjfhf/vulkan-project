all:
	g++ src/main.cpp -o prog

clean:
	rm -f *.o
	rm -f prog
