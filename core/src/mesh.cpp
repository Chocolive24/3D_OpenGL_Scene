#include "mesh.h"
#include "error.h"

#include <algorithm>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<GLuint>& indices) {
  vertices_ = vertices;
  indices_ = indices;

  SetupMesh();
}

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<GLuint>& indices,
           const std::vector<Texture>& textures) {
  vertices_ = vertices;
  indices_ = indices;
  textures_ = textures;

  SetupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept {
  // Transfer ownership of data.
  vertices_ = std::move(other.vertices_);
  indices_ = std::move(other.indices_);
  textures_ = std::move(other.textures_);

  vao_ = std::move(other.vao_);
  vbo_ = std::move(other.vbo_);
  ebo_ = std::move(other.ebo_);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
  // Transfer ownership of data.
  vertices_ = std::move(other.vertices_);
  indices_ = std::move(other.indices_);
  textures_ = std::move(other.textures_);

  vao_ = std::move(other.vao_);
  vbo_ = std::move(other.vbo_);
  ebo_ = std::move(other.ebo_);

  return *this;
}

Mesh::~Mesh() noexcept {
  // if (vao_ != 0) LOG_ERROR("VAO was not destroyed");
  // if (vbo_ != 0) LOG_ERROR("VBO was not destroyed");
  // if (ebo_ != 0) LOG_ERROR("EBO was not destroyed");
}

void Mesh::CreateQuad() noexcept {
  constexpr GLuint kVertexCount = 4;
  vertices_.reserve(kVertexCount);
  vertices_ = {
      Vertex{glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 1.0f)},
      Vertex{glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 0.0f)},
      Vertex{glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 0.0f)},
      Vertex{glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 1.0f)},
  };

  // Create indices.
  constexpr GLuint kIndiceCount = 12;
  indices_.reserve(kIndiceCount);
  indices_ = {
      0, 3, 1, 1, 3, 2, 
      0, 1, 3, 1, 2, 3,
  };

  SetupMesh();
}

void Mesh::CreateCube() noexcept {
  // Create vertices.
  constexpr GLuint kVertexCount = 24;
  vertices_.reserve(kVertexCount);
  vertices_ = {
      // Front face - counterclockwise.
      // right up
      Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 1.0f)},
      // left up
      Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 0.0f)},
      // right down
      Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 0.0f)},

      // Up face - counterclockwise.
      // right up
      Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.f, 1.f, 0.f),
             glm::vec2(1.0f, 1.0f)},
      // left up
      Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.f, 1.f, 0.f),
             glm::vec2(0.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.f, 1.f, 0.f),
             glm::vec2(0.0f, 0.0f)},
      // right down
      Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.f, 1.f, 0.f),
             glm::vec2(1.0f, 0.0f)},

      // Back face - clockwise.
      // left up
      Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.f, 0.f, -1.0f),
             glm::vec2(1.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.f, 0.f, -1.0f),
             glm::vec2(1.0f, 0.0f)},
      // right down
      Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.f, 0.f, -1.0f),
             glm::vec2(0.0f, 0.0f)},
      // right up
      Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.f, 0.f, -1.0f),
             glm::vec2(0.0f, 1.0f)},

      // Down face clockwise.
      // left up
      Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.f, -1.f, 0.f),
             glm::vec2(1.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.f, -1.f, 0.f),
             glm::vec2(1.0f, 0.0f)},
      // right down
      Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.f, -1.f, 0.f),
             glm::vec2(0.0f, 0.0f)},
      // right up
      Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.f, -1.f, 0.f),
             glm::vec2(0.0f, 1.0f)},

      // Right face - counterclockwise.
      // right up
      Vertex{glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1.f, 0.f, 0.f),
             glm::vec2(1.0f, 1.0f)},
      // left up
      Vertex{glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.f, 0.f, 0.f),
             glm::vec2(0.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.f, 0.f, 0.f),
             glm::vec2(0.0f, 0.0f)},
      // right down
      Vertex{glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.f, 0.f, 0.f),
             glm::vec2(1.0f, 0.0f)},

      // Left face - clockwise.
      // left up
      Vertex{glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(-1.f, 0.f, 0.f),
             glm::vec2(0.0f, 1.0f)},
      // left down
      Vertex{glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.f, 0.f, 0.f),
             glm::vec2(0.0f, 0.0f)},
      // right up
      Vertex{glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-1.f, 0.f, 0.f),
             glm::vec2(1.0f, 1.0f)},
      // right down
      Vertex{glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1.f, 0.f, 0.f),
             glm::vec2(1.0f, 0.0f)},
  };

  // Create indices.
  constexpr GLuint kIndiceCount = 36;
  indices_.reserve(kIndiceCount);
  indices_ = {
      0,  1,  3,  1,  2,  3,   // Front face
      4,  5,  7,  5,  6,  7,   // Up face.
      8,  9,  11, 11, 9,  10,  // Back face.
      12, 13, 15, 15, 13, 14,  // Down face.
      16, 17, 19, 17, 18, 19,  // Right face.
      20, 21, 22, 22, 21, 23,  // Left face.
  };

  // Iterates through all the triangles on each face of the cube via their
  // indices to calculate the tangent vector of each vertex.
  for (std::size_t i = 0; i < indices_.size(); i += 3) {
    auto& v1 = vertices_[indices_[i]];
    auto& v2 = vertices_[indices_[i + 1]];
    auto& v3 = vertices_[indices_[i + 2]];

    glm::vec3 edge1 = v2.position - v1.position;
    glm::vec3 edge2 = v3.position - v1.position;
    glm::vec2 deltaUV1 = v2.uv - v1.uv;
    glm::vec2 deltaUV2 = v3.uv - v1.uv;

    // Calculates the tangent vector.
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    v1.tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    v1.tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    v1.tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    v2.tangent = v1.tangent;
    v3.tangent = v1.tangent;

    // The bitangent vectors are calculated in the vertex shader.
  }

  SetupMesh();
}

void Mesh::CreateCubeMap() noexcept {
  // Create vertices.
  constexpr GLuint kVertexCount = 24;
  vertices_.reserve(kVertexCount);
  vertices_ = {
      // Front face - counterclockwise.
      // right up
      Vertex{glm::vec3(1.f, 1.f, 1.f)},
      // left up
      Vertex{glm::vec3(-1.f, 1.f, 1.f)},
      // left down
      Vertex{glm::vec3(-1.f, -1.f, 1.f)},
      // right down
      Vertex{glm::vec3(1.f, -1.f, 1.f)},

      // Up face - counterclockwise.
      // right up
      Vertex{glm::vec3(1.f, 1.f, -1.f)},
      // left up
      Vertex{glm::vec3(-1.f, 1.f, -1.f)},
      // left down
      Vertex{glm::vec3(-1.f, 1.f, 1.f)},
      // right down
      Vertex{glm::vec3(1.f, 1.f, 1.f)},

      // Back face - clockwise.
      // left up
      Vertex{glm::vec3(1.f, 1.f, -1.f)},
      // left down
      Vertex{glm::vec3(1.f, -1.f, -1.f)},
      // right down
      Vertex{glm::vec3(-1.f, -1.f, -1.f)},
      // right up
      Vertex{glm::vec3(-1.f, 1.f, -1.f)},

      // Down face clockwise.
      // left up
      Vertex{glm::vec3(1.f, -1.f, -1.f)},
      // left down
      Vertex{glm::vec3(1.f, -1.f, 1.f)},
      // right down
      Vertex{glm::vec3(-1.f, -1.f, 1.f)},
      // right up
      Vertex{glm::vec3(-1.f, -1.f, -1.f)},

      // Right face - counterclockwise.
      // right up
      Vertex{glm::vec3(1.f, 1.f, -1.f)},
      // left up
      Vertex{glm::vec3(1.f, 1.f, 1.f)},
      // left down
      Vertex{glm::vec3(1.f, -1.f, 1.f)},
      // right down
      Vertex{glm::vec3(1.f, -1.f, -1.f)},

      // Left face - clockwise.
      // left up
      Vertex{glm::vec3(-1.f, 1.f, -1.f)},
      // left down
      Vertex{
          glm::vec3(-1.f, -1.f, -1.f),
      },
      // right up
      Vertex{glm::vec3(-1.f, 1.f, 1.f)},
      // right down
      Vertex{glm::vec3(-1.f, -1.f, 1.f)},
  };

  // Create indices.
  constexpr GLuint kIndiceCount = 36;
  indices_.reserve(kIndiceCount);
  indices_ = {
      0,  1,  3,  1,  2,  3,   // Front face
      4,  5,  7,  5,  6,  7,   // Up face.
      8,  9,  11, 11, 9,  10,  // Back face.
      12, 13, 15, 15, 13, 14,  // Down face.
      16, 17, 19, 17, 18, 19,  // Right face.
      20, 21, 22, 22, 21, 23,  // Left face.
  };

  SetupMesh();
}

void Mesh::CreateScreenQuad() noexcept {
  constexpr GLuint kVertexCount = 4;
  vertices_.reserve(kVertexCount);
  vertices_ = {
      Vertex{glm::vec3(1.f, 1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 1.0f)},
      Vertex{glm::vec3(1.f, -1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 0.0f)},
      Vertex{glm::vec3(-1.f, -1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 0.0f)},
      Vertex{glm::vec3(-1.f, 1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 1.0f)},
  };

  // Create indices.
  constexpr GLuint kIndiceCount = 6;
  indices_.reserve(kIndiceCount);
  indices_ = {0, 1, 3, 1, 2, 3};

  SetupMesh();
}

void Mesh::CreateSphere() noexcept {
  constexpr std::uint8_t kSegmentsX = 64;
  constexpr std::uint8_t kSegmentsY = 64;

  vertices_.reserve((kSegmentsX + 1) * (kSegmentsY + 1));

  for (std::uint8_t x = 0; x <= kSegmentsX; x++) {
    for (std::uint8_t y = 0; y <= kSegmentsY; y++) {
      float x_segment = static_cast<float>(x) / static_cast<float>(kSegmentsX);
      float y_segment = static_cast<float>(y) / static_cast<float>(kSegmentsY);

      // Calculate vertex attributs in spherical coordinates.
      // ----------------------------------------------------
      float theta = 2.0f * M_PI * x_segment;
      float phi = M_PI * y_segment;

      float xPos = std::sin(phi) * std::cos(theta);
      float yPos = std::cos(phi);
      float zPos = std::sin(phi) * std::sin(theta);

      const auto position = glm::vec3(xPos, yPos, zPos);
      const auto normal = glm::normalize(position);
      const auto uv = glm::vec2(x_segment, y_segment);
      glm::vec3 tangent(0.f);
      tangent.x = std::sin(theta) * std::cos(phi);
      tangent.y = std::sin(theta) * std::sin(phi);
      tangent.z = std::cos(theta);

      Vertex v = {
          position,
          normal,  
          uv, 
          tangent,
          // bitangent is calculated in the vertex shader.
      };

      vertices_.push_back(v);
    }
  }

  indices_.reserve(2 * vertices_.size());
  bool odd_row = false;
  for (std::uint8_t y = 0; y < kSegmentsY; y++) {
    if (!odd_row)
    {
      for (std::uint8_t x = 0; x <= kSegmentsX; x++) {
        indices_.push_back(y * (kSegmentsX + 1) + x);
        indices_.push_back((y + 1) * (kSegmentsX + 1) + x);
      }
    } 
    else {
      for (std::int8_t x = kSegmentsX; x >= 0; --x) {
        indices_.push_back((y + 1) * (kSegmentsX + 1) + x);
        indices_.push_back(y * (kSegmentsX + 1) + x);
      }
    }
    odd_row = !odd_row;
  }

  SetupMesh();
}

void Mesh::Destroy() noexcept {
  vao_.Destroy();
  vbo_.Destroy();
  model_matrix_buffer_.Destroy();
  ebo_.Destroy();
}

void Mesh::SetupModelMatrixBuffer(const glm::mat4* model_matrix_data,
                                  const std::size_t& size, GLenum buffer_usage) {
  model_matrix_buffer_.Create();

  vao_.Bind();

  vao_.AttachVBO(model_matrix_buffer_);
  model_matrix_buffer_.SetData(model_matrix_data, size, buffer_usage);

  // Set attribute pointers for matrix (4 times vec4).
  VertexAttributeLayout vertex_layout;
  vertex_layout.PushAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, 1));
  vertex_layout.PushAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, 1));
  vertex_layout.PushAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, 1));
  vertex_layout.PushAttribute(VertexAttribute(4, GL_FLOAT, GL_FALSE, 1));

  vao_.SetupVertexAttributes(vertex_layout);

  vao_.UnBind();
}

void Mesh::SetModelMatrixBufferSubData(const glm::mat4* model_matrix_data, 
                                       const std::size_t& size) noexcept {
  model_matrix_buffer_.Bind();
  model_matrix_buffer_.SetSubData(model_matrix_data, size);
  model_matrix_buffer_.UnBind();
}

void Mesh::GenerateBoundingSphere() {
  glm::vec3 min_aabb = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 max_aabb = glm::vec3(std::numeric_limits<float>::min());

  for (const auto& vertex : vertices_) {
    min_aabb.x = std::min(min_aabb.x, vertex.position.x);
    min_aabb.y = std::min(min_aabb.y, vertex.position.y);
    min_aabb.z = std::min(min_aabb.z, vertex.position.z);

    max_aabb.x = std::max(max_aabb.x, vertex.position.x);
    max_aabb.y = std::max(max_aabb.y, vertex.position.y);
    max_aabb.z = std::max(max_aabb.z, vertex.position.z);
  }

  glm::vec3 center = 0.5f * (min_aabb + max_aabb);
  float radius = glm::length(max_aabb - min_aabb) * 0.5f;

  bounding_volume_ = BoundingSphere(center, radius);
}

void Mesh::SetupMesh() {
  vao_.Create();
  vbo_.Create();
  ebo_.Create();

  // Bind vbo and set its data.
  vao_.AttachVBO(vbo_);
  vbo_.SetData(vertices_.data(), vertices_.size(), GL_STATIC_DRAW);

  // Bind ebo and set its data.
  vao_.AttachEBO(ebo_);
  ebo_.SetData(indices_);

  // Create the input vertex layout.
  VertexAttributeLayout vertex_layout;
  vertex_layout.PushAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE));  // positions.
  vertex_layout.PushAttribute(
      VertexAttribute(3, GL_FLOAT, GL_FALSE));  // normals.
  vertex_layout.PushAttribute(VertexAttribute(2, GL_FLOAT, GL_FALSE));  // uv.
  vertex_layout.PushAttribute(
      VertexAttribute(3, GL_FLOAT, GL_FALSE));  // tangents.
  vertex_layout.PushAttribute(
      VertexAttribute(3, GL_FLOAT, GL_FALSE));  // bitangents.

  vao_.SetupVertexAttributes(vertex_layout);

  vao_.UnBind();
}