#pragma once

#include "mesh.h"
#include "texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string_view>
#include <vector>

class Model {
public:
  Model() = default;

  void Load(std::string_view path, bool gamma = false, bool flip_y = true);
  void Destroy() noexcept;
  void SetupModelMatrixBuffer(const glm::mat4* model_matrix_data,
                              const std::size_t& size, GLenum buffer_usage);
  void SetModelMatrixBufferSubData(const glm::mat4* model_matrix_data,
                                   const std::size_t& size) noexcept;
  void GenerateModelSphereBoundingVolume();

  [[nodiscard]] const std::vector<Mesh>& meshes() const noexcept {
    return meshes_;
  }

private:
  // model data
  std::vector<Texture> textures_loaded_;

  std::vector<Mesh> meshes_;
  std::string directory_;

  
  void ProcessNode(aiNode* node, const aiScene* scene, bool gamma = false, bool flip_y = true);
  Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, bool gamma = false, bool flip_y = true);
  std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type,
                                            std::string typeName, bool gamma = false, 
                                            bool flip_y = true);
};