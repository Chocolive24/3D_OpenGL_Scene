#pragma once

#include "job_system.h"

#include <GL/glew.h>

#include <array>
#include <string_view>

class Texture {
public:
  Texture() = default;

  ~Texture();

  void Create(std::string_view path, GLint wrapping_param,
              GLint filtering_param, bool gamma = false, bool flip_y = true) noexcept;

  void Destroy() noexcept;

  GLuint id = 0;
  std::string type;
  std::string path;
}; 

struct TextureGpu {
  unsigned char* data;
  int width = 0, height = 0, channels = 0;
};

/*
* @brief Load texture from disk to the GPU.
*/
GLuint LoadTexture(std::string_view path, GLint wrapping_param,
                   GLint filtering_param, bool gamma = false, bool flip_y = true);
GLuint LoadHDR_Texture(std::string_view path, GLint wrapping_param,
                       GLint filtering_param, bool flip_y = true);
GLuint LoadCubeMap(const std::array<std::string, 6>& faces, GLint wrapping_param,
                   GLint filtering_param, bool flip_y = false);

// =============================================
//            Multithreading Jobs.
// =============================================

class ImageFileReadingJob final : public Job {
 public:
  ImageFileReadingJob(std::string file_path, FileBuffer* file_buffer) noexcept;

  ImageFileReadingJob(ImageFileReadingJob&& other) noexcept;
  ImageFileReadingJob& operator=(ImageFileReadingJob&& other) noexcept;
  ImageFileReadingJob(const ImageFileReadingJob& other) noexcept = delete;
  ImageFileReadingJob& operator=(const ImageFileReadingJob& other) noexcept = delete;

  ~ImageFileReadingJob() noexcept;

  void Work() noexcept override;

  FileBuffer* file_buffer{};
  std::string file_path{};
};

class ImageFileDecompressingJob final : public Job {
 public:
  ImageFileDecompressingJob(FileBuffer* file_buffer, TextureGpu* texture) noexcept;

  ImageFileDecompressingJob(ImageFileDecompressingJob&& other) noexcept;
  ImageFileDecompressingJob& operator=(ImageFileDecompressingJob&& other) noexcept;
  ImageFileDecompressingJob(const ImageFileDecompressingJob& other) noexcept = delete;
  ImageFileDecompressingJob& operator=(const ImageFileDecompressingJob& other) 
      noexcept = delete;

  ~ImageFileDecompressingJob() noexcept;

  void Work() noexcept override;

 private:
  FileBuffer* file_buffer_;
  TextureGpu* texture_{};
};