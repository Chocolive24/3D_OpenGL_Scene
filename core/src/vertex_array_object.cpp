#include "vertex_array_object.h"
#include "error.h"


VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) noexcept {
  id_ = other.id_;
  attribute_idx_ = other.attribute_idx_;
  other.id_ = 0;
  other.attribute_idx_ = 0;
}

VertexArrayObject& VertexArrayObject::operator=(VertexArrayObject&& other) noexcept {
  id_ = other.id_;
  attribute_idx_ = other.attribute_idx_;
  other.id_ = 0;
  other.attribute_idx_ = 0;

  return *this;
}

VertexArrayObject::~VertexArrayObject() noexcept { 
  if (id_ != 0) {
    LOG_ERROR("VAO was not destroyed.");
  }
}

void VertexArrayObject::Create() noexcept { 
  glCreateVertexArrays(1, &id_); 
}

void VertexArrayObject::Bind() const noexcept { 
  glBindVertexArray(id_);
}

void VertexArrayObject::AttachEBO(const ElementBufferObject& ebo) noexcept {
  Bind();
  ebo.Bind();
  UnBind();
}

void VertexArrayObject::SetupVertexAttributes(
    const VertexAttributeLayout& layout) noexcept {
  Bind();
  int offset = 0;

  for (const auto& attribute : layout.attributes()) {
    glEnableVertexAttribArray(attribute_idx_);
    glVertexAttribPointer(attribute_idx_, attribute.count(), attribute.type(), 
                          attribute.normalized(), layout.stride(), (void*)offset);
    offset += attribute.size();

    if (attribute.divisor() > 0) {
      glVertexAttribDivisor(attribute_idx_, attribute.divisor());
    }

    attribute_idx_++;
  }

  UnBind();
}

void VertexArrayObject::UnBind() const noexcept { 
  glBindVertexArray(0);
}

void VertexArrayObject::Destroy() noexcept { 
  glDeleteVertexArrays(1, &id_); 
  id_ = 0;
  attribute_idx_ = 0;
}