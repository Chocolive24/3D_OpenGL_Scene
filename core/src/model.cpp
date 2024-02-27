#include "model.h"

#include <iostream>

void Model::Destroy() noexcept {
  for (auto& mesh : meshes_) {
    mesh.Destroy();
  }

  for (auto& texture : textures_loaded_) {
    //glDeleteTextures(1, &texture.id);
    texture.Destroy();
  }

  textures_loaded_.clear();
  meshes_.clear();
  directory_ = "";
}

void Model::SetupModelMatrixBuffer(const glm::mat4* model_matrix_data,
                                   const std::size_t& size,
                                   GLenum buffer_usage) { 
  for (auto& mesh : meshes_) {
    mesh.SetupModelMatrixBuffer(model_matrix_data, size, buffer_usage);
  }
}

void Model::SetModelMatrixBufferSubData(const glm::mat4* model_matrix_data, 
                                        const std::size_t& size) noexcept {
  for (auto& mesh : meshes_) {
    mesh.model_matrix_buffer().Bind();
    mesh.SetModelMatrixBufferSubData(model_matrix_data, size);
  }
}

void Model::GenerateModelSphereBoundingVolume() {
  for (auto& mesh : meshes_) {
    mesh.GenerateBoundingSphere();
  }
}

void Model::Load(std::string_view path, bool gamma, bool flip_y) {
  Assimp::Importer import;
  auto flags = aiProcess_Triangulate | aiProcess_CalcTangentSpace;
 
  if (flip_y) {
    flags = flags | aiProcess_FlipUVs;
  }
  
  const aiScene* scene = import.ReadFile(path.data(), flags);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << '\n';
    return;
  }

  directory_ = path.substr(0, path.find_last_of('/'));

  ProcessNode(scene->mRootNode, scene, gamma, flip_y);
}

void Model::LoadToGpu() noexcept {
  for (auto& mesh : meshes_) {
    mesh.LoadToGpu();
  }
}

void Model::ProcessNode(aiNode* node, const aiScene* scene, bool gamma, bool flip_y) {
  // I don't iterates throw all the meshes of the scene directly to be able to
  // set certain mesh as parent of other ones.

  // Process all the node's meshes (if any).
  for (std::size_t i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes_.emplace_back(ProcessMesh(mesh, scene, gamma, flip_y));
  }

  // Do the same for each of its children.
  for (std::size_t i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene, gamma, flip_y);
  }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, bool gamma, bool flip_y) { 
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;
  std::vector<Texture> textures;
  
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    // Process vertex positions, normals and texture coordinates.
    Vertex vertex;

    // Don't convert assimp vector to glm::vector directly because Assimp
    // maintains its own data types for vector, matrices, strings etc, and they
    // don't convert that well to glm's data types.
    vertex.position.x = mesh->mVertices[i].x;
    vertex.position.y = mesh->mVertices[i].y;
    vertex.position.z = mesh->mVertices[i].z;

    vertex.normal.x = mesh->mNormals[i].x;
    vertex.normal.y = mesh->mNormals[i].y;
    vertex.normal.z = mesh->mNormals[i].z;

    // If the mesh contains texture coordinates, stores it.
    if (mesh->mTextureCoords[0]) {
      vertex.uv.x = mesh->mTextureCoords[0][i].x;
      vertex.uv.y = mesh->mTextureCoords[0][i].y;
    } 
    else {
      vertex.uv = glm::vec2(0.0f, 0.0f);
    }

    vertex.tangent.x = mesh->mTangents[i].x;
    vertex.tangent.y = mesh->mTangents[i].y;
    vertex.tangent.z = mesh->mTangents[i].z;

    vertex.tangent = glm::normalize(vertex.tangent);

    vertex.bitangent.x = mesh->mBitangents[i].x;
    vertex.bitangent.y = mesh->mBitangents[i].y;
    vertex.bitangent.z = mesh->mBitangents[i].z;

    vertex.bitangent = glm::normalize(vertex.bitangent);

    vertices.push_back(vertex);
  }

  // Process indices (each faces has a number of indices).
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];

    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // Process material.
  if (mesh->mMaterialIndex >= 0) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // Diffuse maps.
    std::vector<Texture> diffuseMaps = LoadMaterialTextures(material, 
                                                            aiTextureType_DIFFUSE, 
                                                            "texture_diffuse", 
                                                            gamma, flip_y);
    textures.insert(textures.end(),
                    std::make_move_iterator(diffuseMaps.begin()),
                    std::make_move_iterator(diffuseMaps.end()));


    // Specular maps.
    std::vector<Texture> specularMaps = LoadMaterialTextures(material, 
        aiTextureType_SPECULAR, "texture_specular", false, flip_y);
    textures.insert(textures.end(),
                    std::make_move_iterator(specularMaps.begin()),
                    std::make_move_iterator(specularMaps.end()));

    // Normal maps.
    std::vector<Texture> normalMaps = LoadMaterialTextures(material, 
        aiTextureType_HEIGHT, "texture_normal", false, flip_y);

    textures.insert(textures.end(), 
                    std::make_move_iterator(normalMaps.begin()),
                    std::make_move_iterator(normalMaps.end()));
  } 
  
  return std::move(Mesh(std::move(vertices), std::move(indices), 
                        std::move(textures)));
}

std::vector<Texture> Model::LoadMaterialTextures(aiMaterial* mat,
                                                 aiTextureType type,
                                                 std::string type_name,
                                                 bool gamma, bool flip_y) {
  std::vector<Texture> textures;
  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString str;
    mat->GetTexture(type, i, &str);

    bool skip = false;
    for (unsigned int j = 0; j < textures_loaded_.size(); j++) {
      if (std::strcmp(textures_loaded_[j].path.data(), str.C_Str()) == 0) {
        textures.push_back(textures_loaded_[j]);
        skip = true;
        break;
      }
    }

     // If texture hasn't been loaded already, load it.
    if (!skip) {
      std::string texture_path = str.C_Str();
      Texture texture;
      texture.Create(directory_ + '/' + texture_path,
                     GL_REPEAT, GL_LINEAR, gamma, flip_y);
      texture.type = type_name;
      texture.path = texture_path;
      textures_loaded_.push_back(texture);
      textures.push_back(std::move(texture));
    }
  }
  return textures;
}