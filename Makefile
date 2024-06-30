IDIR := ./include
ODIR := .
OBJDIR := ./bin

CC := g++
CFLAGS := -std=c++20
INCLUDES := -I./include -I./thirdparty/glm/
LIBS := -lvulkan `pkg-config --libs --cflags glfw3`
SOURCES := main.cpp engine.cpp vulkan_program.cpp window.cpp
OBJECTS := $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

$(ODIR)/prog: $(OBJECTS)
	$(CC) $^ -o $@ $(LIBS)

$(OBJDIR)/%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o
	rm -f prog
