#pragma once

#include "pipeline.h"

#include <GL/glew.h>
#include <glm/vec3.hpp>

class Material {
 public:
  Material() noexcept = default;
  ~Material() noexcept;

  void Create(const GLuint & albedo, const GLuint & normal, const GLuint & metallic, 
			  const GLuint & roughness, const GLuint & ao);
  void Bind(GLenum gl_texture_idx) const noexcept;

  void Destroy();

  GLuint albedo_map = 0;
  GLuint normal_map = 0;
  GLuint metallic_map = 0;
  GLuint roughness_map = 0;
  GLuint ao_map = 0;
};