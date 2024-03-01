#pragma once

#include "job_system.h"
#include "file_utility.h"

#include <GL/glew.h>

#include <array>
#include <memory>
#include <string_view>
#include <variant>

/*
* @brief TextureParameters is a struct containing the various parameters required 
to create a texture on the GPU.
*/
struct TextureParameters {
  TextureParameters() noexcept = default;
  TextureParameters(std::string_view path, GLint wrap_param, GLint filter_param,
                    bool gamma, bool flip_y, bool hdr = false) noexcept;

  std::string image_file_path{};
  GLint wrapping_param = GL_CLAMP_TO_EDGE;
  GLint filtering_param = GL_LINEAR;
  bool gamma_corrected = false;
  bool flipped_y = false;
  bool hdr = false;
};

/*
* @brief ImageBuffer is a struct containing the data of a decompressed image file.
*/
struct ImageBuffer {
  // Image data can be stored either as unsigned char if it's a classic image, 
  // or as float if it's an image in "hdr" format.
  std::variant<unsigned char*, float*> data; // lifetime is managed by stb_image functions.
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

void LoadTextureToGpu(ImageBuffer* image_buffer, GLuint* id, const TextureParameters& tex_param) noexcept;

// =============================================
//            Multithreading Jobs.
// =============================================

class ImageFileDecompressingJob final : public Job {
 public:
  ImageFileDecompressingJob() noexcept = default;
  ImageFileDecompressingJob(FileBuffer* file_buffer, 
                            ImageBuffer* img_buffer, 
                            bool flip_y = false, bool hdr = false) noexcept;

  void Work() noexcept override;

 private:
  // Shared with loading from disk job.
  FileBuffer* file_buffer_ = nullptr; 
  // Shared with loading texture to GPU job.
  ImageBuffer* image_buffer_ = nullptr;
  bool flip_y_ = false;
  bool hdr_ = false;
};

template <size_t job_count>
class DecompressAllImagesJob final : public Job {
 public:
  explicit DecompressAllImagesJob() noexcept = default;
  explicit DecompressAllImagesJob(
      std::array<ImageFileDecompressingJob, job_count>* decompress_jobs) noexcept :
  decompress_jobs_(decompress_jobs)
  {}

  void Work() noexcept override {
    for (auto& job : *decompress_jobs_) {
      job.Execute();
    }
  }

 private:
  std::array<ImageFileDecompressingJob, job_count>* decompress_jobs_{};
};

class Texture {
 public:
  Texture() = default;

  ~Texture();

  void Create(std::string_view path, GLint wrapping_param,
              GLint filtering_param, bool gamma = false,
              bool flip_y = true) noexcept;

  void Destroy() noexcept;

  GLuint id = 0;
  std::string type;
  std::string path;
};
