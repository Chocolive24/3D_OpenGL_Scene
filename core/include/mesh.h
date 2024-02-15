#pragma once

#include "shapes.h"
#include "texture.h"
#include "vertex_array_object.h"
#include "vertex_buffer_object.h"
#include "vertex_attribute.h"
#include "element_buffer_object.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec3 tangent;
  glm::vec3 bitangent;
};

class Mesh {
public:
  Mesh() noexcept = default;
  Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
  Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices,
       const std::vector<Texture>& textures);
  Mesh(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept;
  Mesh& operator=(const Mesh&) = delete;
  Mesh& operator=(Mesh&& other) noexcept;
  ~Mesh() noexcept;

  void CreateQuad() noexcept;
  void CreateCube() noexcept;
  void CreateCubeMap() noexcept;
  void CreateScreenQuad() noexcept;
  void CreateSphere() noexcept;

  void SetupModelMatrixBuffer(const glm::mat4* model_matrix_data, 
                              const std::size_t& size, GLenum buffer_usage);
  void SetModelMatrixBufferSubData(const glm::mat4* model_matrix_data,
                                   const std::size_t& size) noexcept;
  void GenerateBoundingSphere();

  void Destroy() noexcept;

  [[nodiscard]] const std::vector<Texture>& textures() const noexcept {
    return textures_;
  }

  [[nodiscard]] const VertexArrayObject& vao() const noexcept { return vao_; }

  [[nodiscard]] const std::size_t elementCount() const noexcept {
    return indices_.size();
  }

  [[nodiscard]] const BoundingSphere bounding_sphere() const noexcept {
    return bounding_volume_;
  }

  [[nodiscard]] const VertexBufferObject<glm::mat4>& model_matrix_buffer()
      const noexcept {
    return model_matrix_buffer_;
  }

private:
  std::vector<Vertex> vertices_;
  std::vector<GLuint> indices_;
  std::vector<Texture> textures_;
  VertexArrayObject vao_;
  VertexBufferObject<Vertex> vbo_;
  VertexBufferObject<glm::mat4> model_matrix_buffer_;
  ElementBufferObject ebo_;
  BoundingSphere bounding_volume_;

  void SetupMesh();
};