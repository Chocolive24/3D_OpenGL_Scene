#pragma once

#include "error.h"

#include <GL/glew.h>

#include <vector>

class ElementBufferObject {
public:
  constexpr ElementBufferObject() noexcept = default;
  ElementBufferObject(const ElementBufferObject&) = delete;
  ElementBufferObject(ElementBufferObject&& other) noexcept;
  ElementBufferObject& operator=(const ElementBufferObject&) = delete;
  ElementBufferObject& operator=(ElementBufferObject&& other) noexcept;
  ~ElementBufferObject() noexcept;

  void Create() noexcept;
  void Bind() const noexcept;
  void SetData(const std::vector<GLuint>& indices) noexcept;
  void UnBind() const noexcept;
  void Destroy() noexcept;

  [[nodiscard]] std::size_t element_count() const noexcept {
    return element_count_;
  }

private:
  GLuint id_ = 0;
  std::size_t element_count_ = 0;
};