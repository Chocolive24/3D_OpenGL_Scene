#include "bloom_frame_buffer_object.h"

bool BloomFrameBufferObject::Init(GLuint window_width, GLuint window_height,
                                  GLuint mip_chain_length) {
  if (initialized_) return true;

  glGenFramebuffers(1, &id_);
  glBindFramebuffer(GL_FRAMEBUFFER, id_);

  glm::vec2 mip_size(window_width, window_height);
  glm::ivec2 mip_int_size(window_width, window_height);

  mip_chain_.reserve(mip_chain_length);

  for (unsigned int i = 0; i < mip_chain_length; i++) {
    BloomMip mip{};

    mip_size *= 0.5f;
    mip.size = mip_size;

    glGenTextures(1, &mip.texture);
    glBindTexture(GL_TEXTURE_2D, mip.texture);
    // we are downscaling an HDR color buffer, so we need a float texture format.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mip_size.x, mip_size.y, 0, GL_RGB,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    mip_chain_.push_back(mip);
  }

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         mip_chain_[0].texture, 0);

  // setup attachments
  constexpr std::array<GLuint, 1> attachments = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, attachments.data());

  // check completion status
  int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "G-buffer FBO error, status : " << status << '\n';
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  initialized_ = true;

  return true;
}

void BloomFrameBufferObject::Bind() { glBindFramebuffer(GL_FRAMEBUFFER, id_); }

void BloomFrameBufferObject::Resize(glm::uvec2 new_size) noexcept {
  //glBindTexture(GL_TEXTURE_2D, color_buffer_ids_[i]);
  //const auto& color_attachment = specification_.GetColorAttachment(i);
  //glTexImage2D(GL_TEXTURE_2D, 0, color_attachment.internal_format, new_size.x,
  //             new_size.y, 0, color_attachment.format, GL_UNSIGNED_BYTE, NULL);
}

void BloomFrameBufferObject::Destroy() {
  for (int i = 0; i < mip_chain_.size(); i++) {
    glDeleteTextures(1, &mip_chain_[i].texture);
    mip_chain_[i].texture = 0;
  }

  glDeleteFramebuffers(1, &id_);
  id_ = 0;
  mip_chain_.clear();
  initialized_ = false;
}

