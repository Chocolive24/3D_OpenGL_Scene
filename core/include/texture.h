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

/*
* @brief TextureParameters is a struct containing the various parameters required 
to create a texture on the GPU.
*/
struct TextureParameters {
  TextureParameters(std::string_view path, GLint wrap_param, GLint filter_param,
                    bool gamma, bool flip_y) noexcept
    : image_file_path(path.data()),
      wrapping_param(wrap_param),
      filtering_param(filter_param),
      gamma_corrected(gamma),
      flipped_y(flip_y){};

  std::string image_file_path{};
  GLint wrapping_param = GL_CLAMP_TO_EDGE;
  GLint filtering_param = GL_LINEAR;
  bool gamma_corrected = false;
  bool flipped_y = false;
};

struct ImageBuffer {
  unsigned char* data; // lifetime is managed by stb_image functions.
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

void LoadTextureToGpu(ImageBuffer* image_buffer, GLuint* id, GLint wrapping_param,
                      GLint filtering_param, bool gamma = false) noexcept;

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

  FileBuffer* file_buffer = nullptr;
  std::string file_path{};
};

class ImageFileDecompressingJob final : public Job {
 public:
  ImageFileDecompressingJob(FileBuffer* file_buffer, ImageBuffer* texture, bool flip_y) noexcept;

  ImageFileDecompressingJob(ImageFileDecompressingJob&& other) noexcept;
  ImageFileDecompressingJob& operator=(ImageFileDecompressingJob&& other) noexcept;
  ImageFileDecompressingJob(const ImageFileDecompressingJob& other) noexcept = delete;
  ImageFileDecompressingJob& operator=(const ImageFileDecompressingJob& other) 
      noexcept = delete;

  ~ImageFileDecompressingJob() noexcept;

  void Work() noexcept override;

 private:
  FileBuffer* file_buffer_ = nullptr;
  ImageBuffer* texture_ = nullptr;
  bool flip_y_ = true;
};