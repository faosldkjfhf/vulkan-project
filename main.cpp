#include "engine/engine.h"

int main(int argc, char *argv[]) {
  try {
    bisky::Engine engine;
    engine.run();
  } catch (const std::runtime_error &e) {
    std::cerr << "[exception] " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
