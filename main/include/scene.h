#pragma once

#include "pipeline.h"

#include <GL/glew.h>
#include <SDL.h>

#include <vector>

class Scene {
 public:
  virtual ~Scene() = default;
  virtual void Begin() = 0;
  virtual void End() = 0;
  virtual void Update(float dt) = 0;
  virtual void DrawImGui() {}
  virtual void OnEvent(const SDL_Event& event) {}
};
