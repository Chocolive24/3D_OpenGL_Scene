#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>

#include <array>
#include <vector>
#include <iostream>

struct BloomMip {
  glm::vec2 size;
  GLuint texture;
};

class BloomFrameBufferObject {
 public:
  BloomFrameBufferObject() noexcept = default;
  ~BloomFrameBufferObject() noexcept = default;

  bool Init(GLuint window_width, GLuint window_height, GLuint mip_chain_length);

  void Bind();

  void Resize(glm::uvec2 new_size) noexcept;

  void Destroy();

  [[nodiscard]] const std::vector<BloomMip>& mip_chain() const noexcept {
    return mip_chain_;
  }

 private:
  bool initialized_ = false;
  GLuint id_ = 0;
  std::vector<BloomMip> mip_chain_ = {};
};