#include "element_buffer_object.h"

ElementBufferObject::ElementBufferObject(ElementBufferObject&& other) noexcept {
  id_ = other.id_;
  element_count_ = other.element_count_;

  other.id_ = 0;
  other.element_count_ = 0;
}

ElementBufferObject& ElementBufferObject::operator=(
    ElementBufferObject&& other) noexcept {
  id_ = other.id_;
  element_count_ = other.element_count_;

  other.id_ = 0;
  other.element_count_ = 0;

  return *this;
}

ElementBufferObject::~ElementBufferObject() noexcept { 
  if (id_ != 0) {
    LOG_ERROR("EBO was not destroyed.");
  }
}

void ElementBufferObject::Create() noexcept { 
  glGenBuffers(1, &id_); }

void ElementBufferObject::Bind() const noexcept {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
}

void ElementBufferObject::SetData(const std::vector<GLuint>& indices) noexcept {
  element_count_ = indices.size();
  Bind();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * element_count_,
               indices.data(), GL_STATIC_DRAW);
}

void ElementBufferObject::UnBind() const noexcept {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ElementBufferObject::Destroy() noexcept { 
  glDeleteBuffers(1, &id_);
  id_ = 0;
  element_count_ = 0;
}