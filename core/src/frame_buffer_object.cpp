#include "frame_buffer_object.h"
#include "error.h"

#include <iostream>

ColorAttachment::ColorAttachment(GLint internal_format, GLenum format,
                                 GLint filtering_param, GLint wrapping_param,
                                 std::uint8_t sample_count) noexcept
    : internal_format(internal_format),
      format(format),
      filtering_param(filtering_param),
      wrapping_param(wrapping_param),
      sample_count(sample_count) 
{
}

DepthStencilAttachment::DepthStencilAttachment(GLint internal_format, GLenum format,
                                               std::uint8_t sample_count) noexcept
    : internal_format(internal_format), format(format), sample_count(sample_count) 
{
}

void FrameBufferSpecification::PushColorAttachment(
    const ColorAttachment& color_attach) noexcept {
  color_attachments_[color_attachment_count_] = color_attach;
  color_attachment_count_++;
}

FrameBufferObject::~FrameBufferObject() noexcept {
  if (id_ != 0) {
    LOG_ERROR("FBO was not destroyed.");
  }
}

void FrameBufferObject::Create(
    const FrameBufferSpecification& specification) noexcept {
  glGenFramebuffers(1, &id_);

  Bind();

  specification_ = specification;

  HandleColorAttachments();

  if (specification_.use_depth_stencil()) {
    HandleDepthStencilAttachment();
  }

   // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG_ERROR("Framebuffer not complete !");
  }

  UnBind();
}

void FrameBufferObject::Bind() const noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, id_);
}

void FrameBufferObject::BindRead() const noexcept {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, id_);
}

void FrameBufferObject::BindDraw() const noexcept {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id_);
}

void FrameBufferObject::BindColorBuffer(std::uint8_t idx) const noexcept {
  glBindTexture(GL_TEXTURE_2D, color_buffer_ids_[idx]);
}

void FrameBufferObject::Resize(glm::uvec2 new_size) noexcept {
  specification_.SetSize(new_size);

  if (specification_.use_depth_stencil()) {
    const auto render_buffer_internal_format =
        specification_.depth_stencil_attachment().internal_format;
    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id_);
    glRenderbufferStorage(GL_RENDERBUFFER, render_buffer_internal_format,
                          new_size.x, new_size.y);
  }

  for (std::uint8_t i = 0; i < color_buffer_count_; i++) {
    glBindTexture(GL_TEXTURE_2D, color_buffer_ids_[i]);
    const auto& color_attachment = specification_.GetColorAttachment(i);
    glTexImage2D(GL_TEXTURE_2D, 0, color_attachment.internal_format, new_size.x,
                 new_size.y, 0, color_attachment.format, GL_UNSIGNED_BYTE, NULL);

    GL_CHECK_ERROR();
  }
}

void FrameBufferObject::UnBind() const noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBufferObject::Destroy() noexcept {
  glDeleteFramebuffers(1, &id_);
  glDeleteRenderbuffers(1, &render_buffer_id_);
  id_ = 0;
}

void FrameBufferObject::HandleColorAttachments() {
  const auto size = specification_.size();
  color_buffer_count_ = specification_.color_attachment_count();

  for (std::uint8_t i = 0; i < color_buffer_count_; i++) {
    const auto& color_attachment = specification_.GetColorAttachment(i);
    auto& color_buffer_id = color_buffer_ids_[i];

    glGenTextures(1, &color_buffer_id);

    const GLenum target = color_attachment.sample_count > 1 ? GL_TEXTURE_2D_MULTISAMPLE
                                                            : GL_TEXTURE_2D;

    glBindTexture(target, color_buffer_id);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, 
                    color_attachment.filtering_param);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, 
                    color_attachment.filtering_param);
    glTexParameteri(target, GL_TEXTURE_WRAP_S,
                    color_attachment.wrapping_param);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, 
                    color_attachment.wrapping_param);

    switch (target) {
      case GL_TEXTURE_2D:
        glTexImage2D(GL_TEXTURE_2D, 0, color_attachment.internal_format, size.x,
                     size.y, 0, color_attachment.format, GL_UNSIGNED_BYTE, NULL);
        break;
      case GL_TEXTURE_2D_MULTISAMPLE:
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                color_attachment.sample_count, color_attachment.internal_format,
                                size.x, size.y, GL_TRUE);
        break;
      default:
        break;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i),
                           target, color_buffer_id, 0);
  }

  if (color_buffer_count_ == 0) {
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
  } 
  else {
    static constexpr std::array<GLenum,
                                FrameBufferSpecification::kMaxColorAttachments>
      col_attachments = {
          GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1,  GL_COLOR_ATTACHMENT2,
          GL_COLOR_ATTACHMENT3,  GL_COLOR_ATTACHMENT4,  GL_COLOR_ATTACHMENT5,
          GL_COLOR_ATTACHMENT6,  GL_COLOR_ATTACHMENT7,  GL_COLOR_ATTACHMENT8,
          GL_COLOR_ATTACHMENT9,  GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
          GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14,
          GL_COLOR_ATTACHMENT15,
      };

    glDrawBuffers(static_cast<GLsizei>(color_buffer_count_),
                  col_attachments.data());
  }
}

void FrameBufferObject::HandleDepthStencilAttachment() {
  const auto size = specification_.size();
  const auto& depth_stencil_attachment = specification_.depth_stencil_attachment();

  glGenRenderbuffers(1, &render_buffer_id_);
  glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id_);

  if (depth_stencil_attachment.sample_count == 1) {
    glRenderbufferStorage(GL_RENDERBUFFER,
                          depth_stencil_attachment.internal_format, size.x, size.y);
  } 
  else {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, depth_stencil_attachment.sample_count,
                                     depth_stencil_attachment.internal_format, size.x, size.y);
  }

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, depth_stencil_attachment.format,
                            GL_RENDERBUFFER, render_buffer_id_);
}