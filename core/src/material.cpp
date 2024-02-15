#include "material.h"
#include "error.h"

Material::~Material() noexcept {
  const auto not_destroyed = albedo_map_ != 0 || normal_map_ != 0 ||
                             metallic_map_ != 0 || roughness_map_ != 0 ||
                             ao_map_ != 0;
  if (not_destroyed) {
    LOG_ERROR("Material not destroyed !");
  }
}

void Material::Create(const GLuint& albedo_map, const GLuint& normal_map,
                      const GLuint& metallic_map, const GLuint& roughness_map,
                      const GLuint& ao_map) {
  albedo_map_ = std::move(albedo_map);
  normal_map_ = std::move(normal_map);
  metallic_map_ = std::move(metallic_map);
  roughness_map_ = std::move(roughness_map);
  ao_map_ = std::move(ao_map);
}

void Material::Bind(GLenum gl_texture_idx) const noexcept {
  glActiveTexture(gl_texture_idx);
  glBindTexture(GL_TEXTURE_2D, albedo_map_);
  glActiveTexture(gl_texture_idx + 1);
  glBindTexture(GL_TEXTURE_2D, normal_map_);
  glActiveTexture(gl_texture_idx + 2);
  glBindTexture(GL_TEXTURE_2D, metallic_map_);
  glActiveTexture(gl_texture_idx + 3);
  glBindTexture(GL_TEXTURE_2D, roughness_map_);
  glActiveTexture(gl_texture_idx + 4);
  glBindTexture(GL_TEXTURE_2D, ao_map_);
}

void Material::Destroy() { 
  glDeleteTextures(1, &albedo_map_);
  glDeleteTextures(1, &normal_map_);
  glDeleteTextures(1, &metallic_map_);
  glDeleteTextures(1, &roughness_map_);
  glDeleteTextures(1, &ao_map_);

  albedo_map_ = 0;
  normal_map_ = 0;
  metallic_map_ = 0;
  roughness_map_ = 0;
  ao_map_ = 0;
}
