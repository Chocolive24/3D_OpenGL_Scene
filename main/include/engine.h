#pragma once

#include "scene.h"

class Engine {
 public:
  Engine(Scene* scene);
  void Run();

  static glm::vec2 window_size() { return window_size_; }
  static float window_aspect() { return static_cast<float>(window_size_.x) / 
                                        static_cast<float>(window_size_.y); }
  static void set_clear_color(glm::vec3 new_clear_color) {
    clear_color_ = new_clear_color;
  }

 private:
  void Begin();
  void End();
  Scene* scene_ = nullptr;
  SDL_Window* window_ = nullptr;
  inline static glm::vec2 window_size_ = glm::vec2(1280, 720);
  inline static glm::vec3 clear_color_ = glm::vec3(0);
  SDL_GLContext glRenderContext_{};
};
