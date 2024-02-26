#include "pipeline.h"
#include "mesh.h"
#include "model.h"
#include "error.h"


#include <iostream>
#include <string>

Pipeline::~Pipeline() {
  if (program_ != 0) {
    LOG_ERROR("Program was not destroyed");
  }
}

void Pipeline::Begin(std::string_view vertex_path,
                     std::string_view fragment_path) noexcept {
  // Load vertex shader.
  const auto vertexContent = file_utility::LoadFile(vertex_path);
  const auto* ptr = vertexContent.data();
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &ptr, nullptr);
  glCompileShader(vertex_shader);

  // Check success status of vertex shader compilation
  GLint success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading vertex shader\n";
  }

  // Load fragment shader.
  const auto fragmentContent = file_utility::LoadFile(fragment_path);
  ptr = fragmentContent.data();
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &ptr, nullptr);
  glCompileShader(fragment_shader);

  // Check success status of fragment shader compilation
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading fragment shader\n";
  }

  // Load program/pipeline
  program_ = glCreateProgram();
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);

  // Check if shader program was linked correctly
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    std::cerr << "Error while linking shader program\n";
  }

  // Delete the shaders as they're linked into our program now and no longer
  // necessary.
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void Pipeline::Begin(const FileBuffer& vert_shader_buff,
                     const FileBuffer& frag_shader_buff) noexcept {
  // Load vertex shader.
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  const GLchar* v_shader_source = reinterpret_cast<const GLchar*>(vert_shader_buff.data);

  glShaderSource(vertex_shader, 1, &v_shader_source, &vert_shader_buff.size);
  glCompileShader(vertex_shader);

  // Check success status of vertex shader compilation
  GLint success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading vertex shader.\n";
  }

  // Load fragment shader.
  const GLchar* f_shader_source = reinterpret_cast<const GLchar*>(frag_shader_buff.data);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &f_shader_source, &frag_shader_buff.size);
  glCompileShader(fragment_shader);

  // Check success status of fragment shader compilation
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading fragment shader.\n";
  }

  // Load program/pipeline
  program_ = glCreateProgram();
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);

  // Check if shader program was linked correctly
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    std::cerr << "Error while linking shader program.\n";
  }

  // Delete the shaders as they're linked into our program now and no longer
  // necessary.
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void Pipeline::Bind() const noexcept {
  current_program_ = program_;
  glUseProgram(program_);
}

void Pipeline::End() noexcept {
  glDeleteProgram(program_);
  if (current_program_ == program_) {
    current_program_ = 0;
  }
  program_ = 0;
}

void Pipeline::SetInt(std::string_view name, int value) const noexcept {
  glUniform1i(glGetUniformLocation(program_, name.data()), value);
};

void Pipeline::SetFloat(std::string_view name, float value) const noexcept {
  glUniform1f(glGetUniformLocation(program_, name.data()), value);
};

void Pipeline::SetBool(std::string_view name, bool value) const noexcept {
  glUniform1i(glGetUniformLocation(program_, name.data()), value);
}

void Pipeline::SetVec2(std::string_view name, glm::vec2 vec2) const noexcept {
  glUniform2f(glGetUniformLocation(program_, name.data()), vec2.x, vec2.y);
};

void Pipeline::SetVec3(std::string_view name, glm::vec3 vec3) const noexcept {
  glUniform3f(glGetUniformLocation(program_, name.data()), vec3.x, vec3.y,
              vec3.z);
}

void Pipeline::SetVec4(std::string_view name, glm::vec4 vec4) const noexcept {
  glUniform4f(glGetUniformLocation(program_, name.data()), vec4.x, vec4.y,
              vec4.z, vec4.w);
}

void Pipeline::SetMatrix3(std::string_view name,
                          const glm::mat3& mat) const noexcept {
  glUniformMatrix3fv(glGetUniformLocation(program_, name.data()), 1, GL_FALSE,
                     glm::value_ptr(mat));
};

void Pipeline::SetMatrix4(std::string_view name,
                          const glm::mat4& mat) const noexcept {
  glUniformMatrix4fv(glGetUniformLocation(program_, name.data()), 1, GL_FALSE,
                     glm::value_ptr(mat));
};