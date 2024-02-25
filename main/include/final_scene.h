#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"
#include "material.h"
#include "renderer.h"
#include "frame_buffer_object.h"
#include "bloom_frame_buffer_object.h"
#include "job_system.h"

#include <array>

enum class GeometryPipelineType {
  kGeometry, 
  kShadowMapping, 
  kPointShadowMapping,
};

struct PointLight {
  glm::vec3 position = glm::vec3(0.f);
  glm::vec3 color = glm::vec3(0.f);
  float constant = 0.f;
  float linear = 0.f;
  float quadratic = 0.f;
};

class FinalScene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;
  void DrawImGui() override;

private:
  Renderer renderer_;
  Camera camera_;

  Frustum camera_frustum_;

  glm::mat4 model_, view_, projection_;

  // IBL textures creation pipelines.
  // --------------------------------
  Pipeline equirect_to_cubemap_pipe_;
  Pipeline irradiance_pipeline_;
  Pipeline prefilter_pipeline_;
  Pipeline brdf_pipeline_;

  // Geometry pipelines.
  // -------------------
  Pipeline geometry_pipeline_;
  Pipeline instanced_geometry_pipeline_;
  Pipeline arm_geometry_pipe_;
  Pipeline emissive_arm_geometry_pipe_;
  Pipeline ssao_pipeline_;
  Pipeline ssao_blur_pipeline_;
  Pipeline shadow_mapping_pipe_;
  Pipeline point_shadow_mapping_pipe_;
  Pipeline instanced_shadow_mapping_pipe_;
  Pipeline point_instanced_shadow_mapping_pipe_;

  // Drawing and lighting pipelines.
  // -------------------------------
  Pipeline pbr_lighting_pipeline_;
  Pipeline debug_lights_pipeline_;
  Pipeline cubemap_pipeline_;

  // Postprocessing pipelines.
  // -------------------------
  Pipeline down_sample_pipeline_;
  Pipeline up_sample_pipeline_;
  Pipeline bloom_hdr_pipeline_;

  // Meshes.
  // -------
  Mesh sphere_;
  Mesh cube_;
  Mesh cubemap_mesh_;
  Mesh screen_quad_;

  // Models.
  // -------
  Model leo_magnus_;
  Model sword_;
  Model sandstone_platform_;
  Model treasure_chest_;

  // Materials.
  // ----------
  Material gold_mat_;
  Material sandstone_platform_mat_;

  std::vector<GLuint> leo_magnus_textures_{};
  std::vector<GLuint> sword_textures_{};
  std::vector<GLuint> treasure_chest_textures_{};

  // Frame buffers.
  // --------------
  FrameBufferObject capture_fbo_;
  FrameBufferObject g_buffer_;
  FrameBufferObject ssao_fbo_;
  FrameBufferObject ssao_blur_fbo_;
  GLuint shadow_map_fbo_, shadow_map_;
  GLuint point_shadow_map_fbo_, point_shadow_cubemap_;
  FrameBufferObject hdr_fbo_;

  // IBL textures data.
  // ------------------
  static constexpr std::uint16_t kSkyboxResolution = 4096;
  static constexpr std::uint8_t kIrradianceMapResolution = 32;
  static constexpr std::uint8_t kPrefilterMapResolution = 128;
  static constexpr std::uint16_t kBrdfLutResolution = 512;

  GLuint equirectangular_map_;
  GLuint env_cubemap_;
  GLuint irradiance_cubemap_;
  GLuint prefilter_cubemap_;
  GLuint brdf_lut_;

  // Capture matrices for pre-computing IBL textures.
  // ------------------------------------------------
  const glm::mat4 capture_projection_ = glm::perspective(glm::radians(90.0f), 1.0f, 
                                                         0.1f, 10.f);
  const std::array<glm::mat4, 6> capture_views_ = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))
  };

  // SSAO variables.
  // ---------------
  static constexpr std::uint8_t kSsaoKernelSampleCount_ = 64;
  static constexpr std::uint8_t kSsaoNoiseDimensionX_ = 4;
  static constexpr std::uint8_t kSsaoNoiseDimensionY_ = 4;
  static constexpr float kSsaoRadius = 0.5f;
  static constexpr float kSsaoBiais = 0.025f;
  static constexpr float kCombiendAoFactor = 1.f;

  GLuint noise_texture_;

  std::array<glm::vec3, kSsaoKernelSampleCount_> ssao_kernel_{};

  // Shadow mapping data.
  // --------------------
  static constexpr int kShadowMapWidth_ = 4096, kShadowMapHeight_ = 4096;

  glm::mat4 light_space_matrix_ = glm::mat4(1.f);

  glm::vec3 dir_light_pos_ = glm::vec3(10.0f, 5.f, 10.0f);
  glm::vec3 dir_light_dir_ = glm::normalize(glm::vec3(0) - dir_light_pos_);
  glm::vec3 dir_light_color_ = glm::vec3(4.f, 3.4f, 0.2f);
  bool debug_dir_light_ = false;

  static constexpr int kPointShadowMapRes = 2048;

  static constexpr float kLightNearPlane = 0.1f;
  static constexpr float kLightFarPlane = 50.f;

  glm::mat4 point_light_space_matrix_ = glm::mat4(1.f);

  static constexpr std::array<glm::vec3, 6> light_dirs_ = {
      glm::vec3(1, 0, 0),
      glm::vec3(-1, 0, 0),
      glm::vec3(0, 1, 0),
      glm::vec3(0, -1, 0),
      glm::vec3(0, 0, 1),
      glm::vec3(0, 0, -1),
  };
  static constexpr std::array<glm::vec3, 6> light_ups_ = {
      glm::vec3(0, -1, 0),
      glm::vec3(0, -1, 0),
      glm::vec3(0, 0, 1),
      glm::vec3(0, 0, -1),
      glm::vec3(0, -1, 0),
      glm::vec3(0, -1, 0),
  };

  // Bloom variables.
  // ----------------
  BloomFrameBufferObject bloom_fbo_;
  static constexpr std::uint8_t kBloomMipsCount_ = 5;
  static constexpr float kbloomFilterRadius_ = 0.005f;
  static constexpr float kBloomStrength_ = 0.04f;  // range (0.03, 0.15) works really well.

  // Models variables.
  // -----------------
  static constexpr glm::vec3 treasure_chest_pos_ = glm::vec3(-3.5f, -9.75f, -2.15f);

  // Instancing variables.
  // ---------------------------
  static constexpr std::uint8_t kRowCount_ = 7;
  static constexpr std::uint8_t kColumnCount_ = 7;
  static constexpr std::uint16_t kSphereCount_ = kRowCount_ * kColumnCount_;
  static constexpr float kSpacing_ = 2.5f;

  std::vector<glm::mat4> sphere_model_matrices_{};
  std::vector<glm::mat4> visible_sphere_model_matrices_{};

  // Lights variables.
  // -----------------
  static constexpr std::uint8_t kLightCount = 1;

  std::array<PointLight, kLightCount> point_lights = {
    { glm::vec3(-1.5f, -6.f, 3.0f), glm::vec3(8.f, 0.55f, 8.f), 1.f, 0.09, 0.032f }
  };

  // ImGui variables.
  // ----------------
  bool is_help_window_open_ = true;

  // Begin methods.
  // --------------
  void CreatePipelines() noexcept;

  void CreateMeshes() noexcept;
  void CreateModels() noexcept;
  void CreateMaterials() noexcept;

  void CreateFrameBuffers() noexcept;

  void CreateSsaoData() noexcept;

  void CreateHdrCubemap() noexcept;
  void CreateIrradianceCubeMap() noexcept;
  void CreatePrefilterCubeMap() noexcept;
  void CreateBrdfLut() noexcept;

  // Render passes.
  // --------------
  void ApplyGeometryPass() noexcept;
  void ApplySsaoPass() noexcept;
  void ApplyShadowMappingPass() noexcept;
  void ApplyDeferredPbrLightingPass() noexcept;
  void ApplyFrontShadingPass() noexcept;
  void ApplyBloomPass() noexcept;
  void ApplyHdrPass() noexcept;

  void DrawObjectGeometry(GeometryPipelineType geometry_type) noexcept;
  void DrawInstancedObjectGeometry(GeometryPipelineType geometry_type) noexcept;

  // End methods.
  // ------------
  void DestroyPipelines() noexcept;

  void DestroyMeshes() noexcept;
  void DestroyModels() noexcept;
  void DestroyMaterials() noexcept;

  void DestroyIblPreComputedCubeMaps() noexcept;

  void DestroyFrameBuffers() noexcept;
};

// ===================================================================================
//                              Multithreading.
// ===================================================================================

// Main thread's jobs.
// These jobs are dependent of the OpenGL context so they have
// to be executed by the main thread.
// ----------------------------------
class LoadTextureToGpuJob final : public Job {
 public:
  LoadTextureToGpuJob(ImageBuffer* image_buffer, GLuint* texture_id, 
                      const TextureParameters& tex_param) noexcept;

  LoadTextureToGpuJob(LoadTextureToGpuJob&& other) noexcept;
  LoadTextureToGpuJob& operator=(LoadTextureToGpuJob&& other) noexcept;
  LoadTextureToGpuJob(const LoadTextureToGpuJob& other) noexcept = delete;
  LoadTextureToGpuJob& operator=(const LoadTextureToGpuJob& other) noexcept =
      delete;

  ~LoadTextureToGpuJob() noexcept;

  void Work() noexcept override;

 private:
  ImageBuffer* image_buffer_ = nullptr;
  GLuint* texture_id_ = nullptr;
  TextureParameters texture_param_;
};

// Other thread's jobs.
// --------------------