#pragma once

#include "pipeline.h"

#include <GL/glew.h>
#include <glm/vec3.hpp>

class Material {
 public:
  Material() noexcept = default;
  ~Material() noexcept;

  void Create(const GLuint& albedo_map, const GLuint& normal_map, const GLuint& metallic_map,
              const GLuint& roughness_map, const GLuint& ao_map);
  void Bind(GLenum gl_texture_idx) const noexcept;

  void Destroy();

 private:
  GLuint albedo_map_ = 0;
  GLuint normal_map_ = 0;
  GLuint metallic_map_ = 0;
  GLuint roughness_map_ = 0;
  GLuint ao_map_ = 0;
};