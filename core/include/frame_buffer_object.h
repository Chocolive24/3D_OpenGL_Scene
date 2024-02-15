#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>

#include <array>

struct ColorAttachment {
  GLint internal_format{};
  GLenum format{};
  GLint filtering_param{};
  GLint wrapping_param{};
  std::uint8_t sample_count = 1;

  ColorAttachment() noexcept = default;
  ColorAttachment(GLint internal_format, GLenum format, 
				  GLint filtering_param, GLint wrapping_param, std::uint8_t sample_count = 1) noexcept;
};

struct DepthStencilAttachment {
  GLint internal_format{};
  GLenum format{};
  std::uint8_t sample_count = 1;

  DepthStencilAttachment() noexcept = default;
  DepthStencilAttachment(GLint internal_format, GLenum format,
                         std::uint8_t sample_count = 1) noexcept;
};

class FrameBufferSpecification {
 public:
  FrameBufferSpecification() noexcept = default;

  static constexpr std::uint8_t kMaxColorAttachments = 16;

  void SetSize(glm::uvec2 size) noexcept { size_ = size; }
  void PushColorAttachment(const ColorAttachment& color_attach) noexcept;
  void SetDepthStencilAttachment(const DepthStencilAttachment& 
                                 depth_stencil_attach) noexcept {
    depth_stencil_attachment_ = depth_stencil_attach;
    use_depth_stencil_ = true;
  }

  [[nodiscard]] const glm::uvec2 size() const noexcept { return size_; }
  
  [[nodiscard]] const std::uint8_t color_attachment_count() const noexcept {
    return color_attachment_count_;
  }
  
  [[nodiscard]] const ColorAttachment& GetColorAttachment(GLuint idx ) const noexcept {
    return color_attachments_[idx];
  }
  
  [[nodiscard]] const DepthStencilAttachment& depth_stencil_attachment()
      const noexcept {
    return depth_stencil_attachment_;
  }

  [[nodiscard]] const bool use_depth_stencil() const noexcept {
    return use_depth_stencil_;
  }

 private:
  glm::uvec2 size_ = glm::uvec2(0);
  std::uint8_t color_attachment_count_ = 0;
  std::array<ColorAttachment, kMaxColorAttachments> color_attachments_{};
  DepthStencilAttachment depth_stencil_attachment_{};

  bool use_depth_stencil_ = false;
};

class FrameBufferObject {
public:
  constexpr FrameBufferObject() noexcept = default;
  ~FrameBufferObject() noexcept;

  void Create(const FrameBufferSpecification& specification) noexcept;
  void Bind() const noexcept;
  void BindRead() const noexcept;
  void BindDraw() const noexcept;
  void BindColorBuffer(std::uint8_t idx) const noexcept;

  void Resize(glm::uvec2 new_size) noexcept;

  void UnBind() const noexcept;
  void Destroy() noexcept;

private:
  GLuint id_ = 0;

  FrameBufferSpecification specification_{};

  std::uint8_t color_buffer_count_ = 0;
  std::array<GLuint, FrameBufferSpecification::kMaxColorAttachments>
      color_buffer_ids_{};
  
  GLuint render_buffer_id_ = 0;

  void HandleColorAttachments();
  void HandleDepthStencilAttachment();
};