#pragma once

#include "error.h"

#include "GL/glew.h"

#include <vector>

/*
* @brief VertexAttribute is a class that contains the data of an input vertex.
*/
class VertexAttribute {
 public:
  constexpr VertexAttribute(GLuint count, GLenum type,
                            GLboolean normalized, GLuint divisor = 0) noexcept
      : count_(count), type_(type), normalized_(normalized), 
        size_(count_ * GetSizeOfType()),
        divisor_(divisor) 
  {}

  [[nodiscard]] constexpr GLuint count() const noexcept { return count_; }
  [[nodiscard]] constexpr GLenum type() const noexcept { return type_; }
  [[nodiscard]] constexpr GLboolean normalized() const noexcept { return normalized_; }
  [[nodiscard]] constexpr GLsizei size() const noexcept { return size_; }
  [[nodiscard]] constexpr GLuint divisor() const noexcept { return divisor_; }

 private:
  GLuint count_;
  GLenum type_;
  GLboolean normalized_;
  GLsizei size_;
  GLuint divisor_ = 0;

  [[nodiscard]] constexpr GLsizei GetSizeOfType() const noexcept {
    switch (type_) {
      case GL_FLOAT:
        return sizeof(GLfloat);
      case GL_INT:
        return sizeof(GLint);
      default:
        LOG_ERROR("Incorrect vertex attribute type.");
    }
  }
};

/*
* @brief VertexAttributeLayout is a class that stores and helps to communicate the layout of the 
* inputs vertex data to a VertexArrayObject.
*/
class VertexAttributeLayout {
 public:
  void PushAttribute(const VertexAttribute& attribute) noexcept;
  void Clear() noexcept;

  [[nodiscard]] const std::vector<VertexAttribute>& attributes() const noexcept {
    return attributes_;
  }

  [[nodiscard]] const GLsizei stride() const noexcept {
    return stride_;
  }

 private:
  // Vertex shader location of each attribute is its index in the vector.
  std::vector<VertexAttribute> attributes_;
  GLsizei stride_ = 0;
};