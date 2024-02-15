#pragma once

#include "error.h"

#include "GL/glew.h"

#include <vector>

template <typename T>
class VertexBufferObject {
 public:
  constexpr VertexBufferObject() noexcept = default;
  VertexBufferObject(const VertexBufferObject&) = delete;
  VertexBufferObject(VertexBufferObject&& other) noexcept;
  VertexBufferObject& operator=(const VertexBufferObject&) = delete;
  VertexBufferObject& operator=(VertexBufferObject&& other) noexcept;
  ~VertexBufferObject() noexcept;

  void Create() noexcept;
  void Bind() const noexcept;
  void SetData(const T* vertex_data, const std::size_t& size, GLenum usage) noexcept;
  void SetSubData(const T* vertex_data, const std::size_t& size) noexcept;
  void UnBind() const noexcept;
  void Destroy() noexcept;

  [[nodiscard]] const GLuint id() const noexcept { return id_; }

 private:
  GLuint id_ = 0;
};

template <typename T>
inline VertexBufferObject<T>::VertexBufferObject(
  VertexBufferObject&& other) noexcept {
  id_ = other.id_;
  other.id_ = 0;
}

template <typename T>
inline VertexBufferObject<T>& VertexBufferObject<T>::operator=(
    VertexBufferObject&& other) noexcept {
  id_ = other.id_;
  other.id_ = 0;

  return *this;
}

template <typename T>
inline VertexBufferObject<T>::~VertexBufferObject() noexcept {
  if (id_ != 0) {
    LOG_ERROR("VBO was not destroyed.")
  }
}

template <typename T>
inline void VertexBufferObject<T>::Create() noexcept {
  glGenBuffers(1, &id_);
}

template <typename T>
inline void VertexBufferObject<T>::Bind() const noexcept {
  glBindBuffer(GL_ARRAY_BUFFER, id_);
}

template <typename T>
inline void VertexBufferObject<T>::SetData(const T* vertex_data, 
                                           const std::size_t& size, 
                                           GLenum usage) noexcept {
  glBufferData(GL_ARRAY_BUFFER, sizeof(T) * size,
               vertex_data, usage);
}

template <typename T>
inline void VertexBufferObject<T>::SetSubData(const T* vertex_data, 
                                              const std::size_t& size) noexcept {
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(T) * size,
                  vertex_data);
}

template <typename T>
inline void VertexBufferObject<T>::UnBind() const noexcept {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template <typename T>
inline void VertexBufferObject<T>::Destroy() noexcept {
  glDeleteBuffers(1, &id_);
  id_ = 0;
}
