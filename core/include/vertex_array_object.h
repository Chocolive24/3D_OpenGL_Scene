#pragma once

#include "vertex_buffer_object.h"
#include "element_buffer_object.h"
#include "vertex_attribute.h"

#include <GL/glew.h>
#include <glm/vec3.hpp>

#include <vector>

class VertexArrayObject {
public:
  constexpr VertexArrayObject() noexcept = default;
  VertexArrayObject(const VertexArrayObject& other) = delete;
  VertexArrayObject(VertexArrayObject&& other) noexcept;
  VertexArrayObject& operator=(const VertexArrayObject& other) = delete;
  VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;
  ~VertexArrayObject() noexcept;

  void Create() noexcept;
  void Bind() const noexcept;

  template <typename T>
  void AttachVBO(const VertexBufferObject<T>& vbo) noexcept;

  void AttachEBO(const ElementBufferObject& ebo) noexcept;

  void SetupVertexAttributes(const VertexAttributeLayout& layout) noexcept;

  void UnBind() const noexcept;
  void Destroy() noexcept;

  [[nodiscard]] const GLuint id() const noexcept { return id_; }

private:
  GLuint id_ = 0;
  GLuint attribute_idx_ = 0;
};

template <typename T>
void VertexArrayObject::AttachVBO(const VertexBufferObject<T>& vbo) noexcept {
  Bind();
  vbo.Bind();
  UnBind();
}