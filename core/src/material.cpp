#include "material.h"
#include "error.h"

Material::~Material() noexcept {
  const auto not_destroyed = albedo_map != 0 || normal_map != 0 ||
                             metallic_map != 0 || roughness_map != 0 ||
                             ao_map != 0;
  if (not_destroyed) {
    LOG_ERROR("Material not destroyed !");
  }
}

void Material::Create(const GLuint & albedo, const GLuint & normal, const GLuint & metallic, 
                      const GLuint & roughness, const GLuint & ao) {
  albedo_map = std::move(albedo);
  normal_map = std::move(normal);
  metallic_map = std::move(metallic);
  roughness_map = std::move(roughness);
  ao_map = std::move(ao);
}

void Material::Bind(GLenum gl_texture_idx) const noexcept {
  glActiveTexture(gl_texture_idx);
  glBindTexture(GL_TEXTURE_2D, albedo_map);
  glActiveTexture(gl_texture_idx + 1);
  glBindTexture(GL_TEXTURE_2D, normal_map);
  glActiveTexture(gl_texture_idx + 2);
  glBindTexture(GL_TEXTURE_2D, metallic_map);
  glActiveTexture(gl_texture_idx + 3);
  glBindTexture(GL_TEXTURE_2D, roughness_map);
  glActiveTexture(gl_texture_idx + 4);
  glBindTexture(GL_TEXTURE_2D, ao_map);
}

void Material::Destroy() { 
  glDeleteTextures(1, &albedo_map);
  glDeleteTextures(1, &normal_map);
  glDeleteTextures(1, &metallic_map);
  glDeleteTextures(1, &roughness_map);
  glDeleteTextures(1, &ao_map);

  albedo_map = 0;
  normal_map = 0;
  metallic_map = 0;
  roughness_map = 0;
  ao_map = 0;
}
