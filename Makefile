all:
	g++ --std=c++20 `pkg-config --cflags glfw3 gl` src/main.cpp -o prog `pkg-config --libs glfw3 gl`

clean:
	rm -f *.o
	rm -f prog
