#pragma once

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
* @brief Load texture from disk to the GPU.
*/
GLuint LoadTexture(std::string_view path, GLint wrapping_param,
                   GLint filtering_param, bool gamma = false, bool flip_y = true);
GLuint LoadHDR_Texture(std::string_view path, GLint wrapping_param,
                       GLint filtering_param, bool flip_y = true);
GLuint LoadCubeMap(const std::array<std::string, 6>& faces, GLint wrapping_param,
                   GLint filtering_param, bool flip_y = false);