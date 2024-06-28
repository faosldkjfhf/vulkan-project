all:
	bear -- g++ --std=c++20 `pkg-config --cflags glfw3` -I./include src/*.cpp -o prog -lvulkan `pkg-config --libs glfw3`

clean:
	rm -f *.o
	rm -f prog
