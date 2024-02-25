#include "texture.h"
#include "error.h"
#include "file_utility.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif


#ifdef TRACY_ENABLE
#include <TracyC.h>

#include <Tracy.hpp>
#endif  // TRACY_ENABLE
#include <stb_image.h>

#include <iostream>

TextureParameters::TextureParameters(std::string_view path, GLint wrap_param,
                                     GLint filter_param, bool gamma,
                                     bool flip_y, bool hdr) noexcept
  : image_file_path(path.data()),
    wrapping_param(wrap_param),
    filtering_param(filter_param),
    gamma_corrected(gamma),
    flipped_y(flip_y),
    hdr(hdr)
{
};

ImageFileDecompressingJob::ImageFileDecompressingJob(
    FileBuffer* file_buffer, ImageBuffer* texture, bool flip_y, bool hdr) noexcept
    : Job(JobType::kFileDecompressing),
      file_buffer_(file_buffer),
      texture_(texture),
      flip_y_(flip_y),
      hdr_(hdr)
{
}

ImageFileDecompressingJob::ImageFileDecompressingJob(
    ImageFileDecompressingJob&& other) noexcept
    : Job(std::move(other)) {
  file_buffer_ = std::move(other.file_buffer_);
  texture_ = std::move(other.texture_);
  flip_y_ = std::move(other.flip_y_);
  hdr_ = std::move(other.hdr_);

  other.file_buffer_ = nullptr;
  other.texture_ = nullptr;
}

ImageFileDecompressingJob& ImageFileDecompressingJob::operator=(
    ImageFileDecompressingJob&& other) noexcept {
  Job::operator=(std::move(other));
  file_buffer_ = std::move(other.file_buffer_);
  texture_ = std::move(other.texture_);
  flip_y_ = std::move(other.flip_y_);
  hdr_ = std::move(other.hdr_);

  other.file_buffer_ = nullptr;
  other.texture_ = nullptr;

  return *this;
}

ImageFileDecompressingJob::~ImageFileDecompressingJob() {
  file_buffer_ = nullptr;
  texture_ = nullptr;
}

void ImageFileDecompressingJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  stbi_set_flip_vertically_on_load(flip_y_);

  if (hdr_) {
    texture_->data = stbi_loadf_from_memory(file_buffer_->data, file_buffer_->size,
                                            &texture_->width, &texture_->height,
                                            &texture_->channels, 0);
  } 
  else {
    texture_->data = stbi_load_from_memory(file_buffer_->data, file_buffer_->size,
                                          &texture_->width, &texture_->height,
                                          &texture_->channels, 0);
  }

}

GLuint LoadTexture(std::string_view path, GLint wrapping_param, 
                    GLint filtering_param, bool gamma, bool flip_y) { 
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  // Load texture.
  int width, height, channels;

  stbi_set_flip_vertically_on_load(flip_y);

#ifdef TRACY_ENABLE
  ZoneNamedN(Read, "Read Texture File.", true);
#endif

  const FileBuffer file_buffer = file_utility::LoadFileBuffer(path.data());

#ifdef TRACY_ENABLE
  ZoneNamedN(UnCompress, "UnCompress Texture File.", true);
#endif
  const auto texture_uncompress = stbi_load_from_memory(file_buffer.data, 
      file_buffer.size, &width, &height, &channels, 0);

  if (texture_uncompress == nullptr) {
    std::cerr << "Error in loading the image at path " << path << '\n';
  }

#ifdef TRACY_ENABLE
  ZoneNamedN(UploadTexToGPU, "Upload texture to GPU.", true);
#endif
  // Give texture to GPU.
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping_param);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_param);

  GLint internal_format = GL_RGB;
  GLenum format = GL_RGB;
  if (channels == 1) {
    internal_format = GL_RED;
    format = GL_RED;
  } 
  else if (channels == 2) {
    internal_format = GL_RG;
    format = GL_RG;
  } 
  else if (channels == 3) {
    internal_format = gamma ? GL_SRGB : GL_RGB;
    format = GL_RGB;
  } 
  else if (channels == 4) {
    internal_format = gamma ? GL_SRGB_ALPHA : GL_RGBA;
    format = GL_RGBA;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
               format, GL_UNSIGNED_BYTE, texture_uncompress);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(texture_uncompress);

  return texture;
}

GLuint LoadHDR_Texture(std::string_view path, GLint wrapping_param,
                       GLint filtering_param, bool flip_y) {
  // Load texture.
  int width, height, channels;

  stbi_set_flip_vertically_on_load(flip_y);
  auto texture_data = stbi_loadf(path.data(), &width, &height, &channels, 0);

  if (texture_data == nullptr) {
    std::cerr << "Error in loading the image at path " << path << '\n';
  }

  std::cout << "Loaded image with a width of " << width << "px, a height of "
            << height << "px, and " << channels << "channels \n";

  // Give texture to GPU.
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT,
               texture_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping_param);

  /*GLint min_filter_param = filtering_param == GL_LINEAR
      ? GL_LINEAR_MIPMAP_LINEAR : filtering_param;*/

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_param);

  stbi_image_free(texture_data);

  return texture;
}

GLuint LoadCubeMap(const std::array<std::string, 6>& faces, GLint wrapping_param,
                   GLint filtering_param, bool flip_y) {
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

  int width, height, channels;
  stbi_set_flip_vertically_on_load(flip_y);

  for (std::size_t i = 0; i < faces.size(); i++) {
    auto* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

    if (data) {
      GLint internalFormat = channels == 3 ? GL_RGB : GL_RGBA;

      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, internalFormat, GL_UNSIGNED_BYTE, data);

      std::cout << "Loaded image with a width of " << width
                << "px, a height of " << height << "px, and " << channels
                << "channels \n";

    } 
    else {
      std::cout << "Cubemap tex failed to load at path: " << faces[i]
                << std::endl;
    }

    stbi_image_free(data);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapping_param);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapping_param);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapping_param);

  return texture_id;
}

void LoadTextureToGpu(ImageBuffer* image_buffer, GLuint* id,
                      const TextureParameters& tex_param) noexcept {
  glGenTextures(1, id);
  glBindTexture(GL_TEXTURE_2D, *id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex_param.wrapping_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex_param.wrapping_param);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex_param.filtering_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex_param.filtering_param);

  GLint internal_format = GL_RGB;
  GLenum format = GL_RGB;

  switch (image_buffer->channels) { 
    case 1:
      internal_format = GL_RED;
      format = GL_RED;
      break;
    case 2:
      internal_format = GL_RG;
      format = GL_RG;
      break;
    case 3:
      if (tex_param.hdr) {
        internal_format = GL_RGB16F;
      } 
      else {
        internal_format = tex_param.gamma_corrected ? GL_SRGB : GL_RGB;
      }
      format = GL_RGB;
      break;
    case 4:
      internal_format = tex_param.gamma_corrected ? GL_SRGB_ALPHA : GL_RGBA;
      format = GL_RGBA;
      break;
    default:
      break;
  }

  if (tex_param.hdr) {
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image_buffer->width,
                 image_buffer->height, 0, format, GL_FLOAT,
                 std::get<float*>(image_buffer->data));
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(std::get<float*>(image_buffer->data));
  } 
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image_buffer->width,
                 image_buffer->height, 0, format, GL_UNSIGNED_BYTE,
                 std::get<unsigned char*>(image_buffer->data));
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(std::get<unsigned char*>(image_buffer->data));
  }
}






// Texture::Texture(const Texture& other) {}
//
// Texture::Texture(Texture&& other) noexcept {
//   if (this != &other) {
//     id = other.id;
//     type = std::move(other.type);
//     path = std::move(other.path);
//
//     other.id = 0;
//   }
// }
//
// Texture& Texture::operator=(const Texture&) { return *this; }
//
// Texture& Texture::operator=(Texture&& other) noexcept {
//   if (this != &other) {
//     id = other.id;
//     type = std::move(other.type);
//     path = std::move(other.path);
//
//     other.id = 0;
//   }
//   return *this;
// }

Texture::~Texture() {
  // if (id != 0) {
  //   LOG_ERROR("Texture " + path + " not destroyed.");
  // }
}

void Texture::Create(std::string_view path, GLint wrapping_param,
                     GLint filtering_param, bool gamma, bool flip_y) noexcept {
  // Load texture.
  int width, height, channels;

  stbi_set_flip_vertically_on_load(flip_y);
  auto texture_data = stbi_load(path.data(), &width, &height, &channels, 0);

  if (texture_data == nullptr) {
    std::cerr << "Error in loading the image at path " << path << '\n';
    std::exit(1);
  }

  std::cout << "Loaded image with a width of " << width << "px, a height of "
            << height << "px, and " << channels << "channels \n";

  // Give texture to GPU.
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  GLint internal_format;
  GLenum format;

  if (channels == 3) {
    internal_format = gamma ? GL_SRGB : GL_RGB;
    format = GL_RGB;
  } else if (channels == 4) {
    internal_format = gamma ? GL_SRGB_ALPHA : GL_RGBA;
    format = GL_RGBA;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format,
               GL_UNSIGNED_BYTE, texture_data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping_param);

  /*GLint min_filter_param = filtering_param == GL_LINEAR
      ? GL_LINEAR_MIPMAP_LINEAR : filtering_param;*/

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_param);

  stbi_image_free(texture_data);
}

void Texture::Destroy() noexcept {
  glDeleteTextures(1, &id);
  id = 0;
}