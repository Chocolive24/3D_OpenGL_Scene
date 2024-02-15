#include "vertex_attribute.h"

void VertexAttributeLayout::PushAttribute(
    const VertexAttribute& attribute) noexcept {
  attributes_.push_back(attribute);
  stride_ += attribute.size();
}

void VertexAttributeLayout::Clear() noexcept { 
  attributes_.clear();
  stride_ = 0;
}