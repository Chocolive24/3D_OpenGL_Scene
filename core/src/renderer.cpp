#include "renderer.h"

void Renderer::DrawMesh(const Mesh& mesh, GLenum mode) const noexcept {
  glBindVertexArray(mesh.vao().id());
  glDrawElements(mode, mesh.elementCount(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Renderer::DrawInstancedMesh(const Mesh& mesh, GLuint instance_count, 
                                 GLenum mode) const noexcept {
  glBindVertexArray(mesh.vao().id());
  glDrawElementsInstanced(mode, mesh.elementCount(), GL_UNSIGNED_INT, 0,
                          instance_count);
  glBindVertexArray(0);
}

void Renderer::DrawModel(const Model& model, GLenum mode) const noexcept {
  for (const auto& mesh : model.meshes())
  {
    DrawMesh(mesh, mode);
  }
}

void Renderer::DrawInstancedModel(const Model& model, GLuint instance_count,
                                  GLenum mode) const noexcept {
  for (const auto& mesh : model.meshes()) {
    DrawInstancedMesh(mesh, instance_count, mode);
  }
}

void Renderer::DrawModelWithMaterials(const Model& model, 
                                      const std::vector<GLuint>& textures, 
                                      int mat_offset, GLenum mode) {
  int offset = 0;
  for (const auto& mesh : model.meshes()) {
    for (int i = 0; i < 4; i++) {
      if (i >= textures.size()) {
        continue;
      }

      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, textures[i + offset]);
    }

    DrawMesh(mesh, mode);
    offset += mat_offset;
  }
}
