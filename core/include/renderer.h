#pragma once

#include "mesh.h"
#include "model.h"
#include "material.h"
#include "pipeline.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

class Renderer {
public:
  void DrawMesh(const Mesh& mesh, GLenum mode = GL_TRIANGLES) const noexcept;
  void DrawInstancedMesh(const Mesh& mesh,
                         GLuint instance_count, GLenum mode = GL_TRIANGLES) const noexcept;
  void DrawModel(const Model& model, GLenum mode = GL_TRIANGLES) const noexcept;
  void DrawInstancedModel(const Model& model, GLuint instance_count,
                          GLenum mode = GL_TRIANGLES) const noexcept;

  void DrawModelWithMaterials(const Model& model, const std::vector<GLuint>& textures, 
                              int mat_offset, GLenum mode = GL_TRIANGLES);
};