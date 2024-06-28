#include <GLFW/glfw3.h>
#include <iostream>

static void glfwError(int id, const char *description) {
  std::cout << description << std::endl;
}

int main() {
  GLFWwindow *_window;

  glfwSetErrorCallback(&glfwError);
  if (!glfwInit()) {
    std::cout << "Failed to init" << std::endl;
    return -1;
  }

  _window = glfwCreateWindow(640, 480, "Vulkan Program", NULL, NULL);
  if (!_window) {
    std::cout << "Window failed to create" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(_window);

  while (!glfwWindowShouldClose(_window)) {
    /* Poll for and process events */
    glfwPollEvents();

    /* Render here */
    glClear(GL_COLOR_BUFFER_BIT);

    /* Swap front and back buffers */
    glfwSwapBuffers(_window);
  }

  glfwTerminate();
  return 0;
}
