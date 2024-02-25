#pragma once

#include "job_system.h"

#include <GL/glew.h>

#include <array>
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
  // Image data can be stored either as an unsigned char if it's a classic image, 
  // or as a float if it's an image in "hdr" format.
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
  ImageFileDecompressingJob(FileBuffer* file_buffer, ImageBuffer* texture, 
                            bool flip_y = false, bool hdr = false) noexcept;

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
  bool flip_y_ = false;
  bool hdr_ = false;
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