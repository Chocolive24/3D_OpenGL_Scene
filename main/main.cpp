#include "engine.h"
#include "final_scene.h"

#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  FinalScene scene;
  Engine engine(&scene);
  engine.Run();

  return EXIT_SUCCESS;
}