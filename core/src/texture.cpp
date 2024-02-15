#include "texture.h"
#include "error.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

//Texture::Texture(const Texture& other) {}
//
//Texture::Texture(Texture&& other) noexcept { 
//  if (this != &other) {
//    id = other.id;
//    type = std::move(other.type);
//    path = std::move(other.path);
//
//    other.id = 0;
//  }
//}
//
//Texture& Texture::operator=(const Texture&) { return *this; }
//
//Texture& Texture::operator=(Texture&& other) noexcept {
//  if (this != &other) {
//    id = other.id;
//    type = std::move(other.type);
//    path = std::move(other.path);
//
//    other.id = 0;
//  }
//  return *this;
//}

Texture::~Texture() { 
  //if (id != 0) {
  //  LOG_ERROR("Texture " + path + " not destroyed.");
  //}
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

GLuint LoadTexture(std::string_view path, GLint wrapping_param, 
                    GLint filtering_param, bool gamma, bool flip_y) { 
  // Load texture.
  int width, height, channels;

  stbi_set_flip_vertically_on_load(flip_y);
  auto texture_data = stbi_load(path.data(), &width,
                                &height, &channels, 0);

  if (texture_data == nullptr) {
    std::cerr << "Error in loading the image at path " << path << '\n';
  }

  std::cout << "Loaded image with a width of " << width
            << "px, a height of " << height << "px, and "
            << channels << "channels \n";

  // Give texture to GPU.
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping_param);

  /*GLint min_filter_param = filtering_param == GL_LINEAR
      ? GL_LINEAR_MIPMAP_LINEAR : filtering_param;*/

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_param);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_param);

  GLint internal_format;
  GLenum format;
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
               format, GL_UNSIGNED_BYTE, texture_data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(texture_data);

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