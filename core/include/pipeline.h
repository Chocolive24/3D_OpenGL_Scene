#pragma once

#include "mesh.h"
#include "model.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>

class Pipeline {
 public:
  ~Pipeline();

  void Begin(std::string_view vertex_path, std::string_view fragment_path) noexcept;

  void Bind() const noexcept;
  void DrawMesh(const Mesh& mesh, GLenum mode = GL_TRIANGLES, 
                GLuint start_tex_unit = 0) noexcept;
  void DrawModel(const Model& model, GLuint start_tex_unit = 0) noexcept;

  void End() noexcept;

  void Pipeline::SetInt(std::string_view name, int value) const noexcept;
  void Pipeline::SetFloat(std::string_view name, float value) const noexcept;
  void Pipeline::SetBool(std::string_view name, bool value) const noexcept;
  void Pipeline::SetVec2(std::string_view name, glm::vec2 vec2) const noexcept;
  void Pipeline::SetVec3(std::string_view name, glm::vec3 vec3) const noexcept;
  void Pipeline::SetVec4(std::string_view name, glm::vec4 vec3) const noexcept;
  void Pipeline::SetMatrix3(std::string_view name, 
                            const glm::mat3& mat) const noexcept;
  void Pipeline::SetMatrix4(std::string_view name,
                            const glm::mat4& mat) const noexcept;

  [[nodiscard]] static const GLuint current_program() noexcept {
    return current_program_;
  }

private:
  GLuint program_ = 0;
  inline static GLuint current_program_ = 0;
};