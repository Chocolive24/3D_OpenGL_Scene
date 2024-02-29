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

// ===================================================================================
//                              Multithreading.
// ===================================================================================

// Main thread's jobs.
// These jobs are dependent of the OpenGL context so they have
// to be executed by the main thread.
// ----------------------------------
class PipelineCreationJob final : public Job {
 public:
   PipelineCreationJob() noexcept = default;
   PipelineCreationJob(FileBuffer* v_shader_buff,
                       FileBuffer* f_shader_buff,
                       Pipeline* pipeline);
   PipelineCreationJob(PipelineCreationJob&& other) noexcept = default;
   PipelineCreationJob& operator=(PipelineCreationJob&& other) noexcept = default;
   PipelineCreationJob(const PipelineCreationJob& other) noexcept =
       delete;
   PipelineCreationJob& operator=(
       const PipelineCreationJob& other) noexcept = delete;
   ~PipelineCreationJob() noexcept override = default;

   void Work() noexcept override;

 private:
   // Shared with the load shader file from disk job.
   FileBuffer* vertex_shader_buffer_ = nullptr;
   FileBuffer* fragment_shader_buffer_ = nullptr;
   Pipeline* pipeline_ = nullptr;
};

class LoadTextureToGpuJob final : public Job {
 public:
  LoadTextureToGpuJob() noexcept = default;
  LoadTextureToGpuJob(ImageBuffer* image_buffer,
                      GLuint* texture_id,
                      const TextureParameters& tex_param) noexcept;
  LoadTextureToGpuJob(LoadTextureToGpuJob&& other) noexcept = default;
  LoadTextureToGpuJob& operator=(LoadTextureToGpuJob&& other) noexcept = default;
  LoadTextureToGpuJob(const LoadTextureToGpuJob& other) noexcept = delete;
  LoadTextureToGpuJob& operator=(const LoadTextureToGpuJob& other) noexcept =
      delete;
  ~LoadTextureToGpuJob() noexcept override = default;

  void Work() noexcept override;

 private:
  // Shared with the image decompressing job.
  ImageBuffer* image_buffer_ = nullptr;
  GLuint* texture_id_ = nullptr;
  TextureParameters texture_param_;
};

class FunctionExecutionJob final : public Job {
public:
  FunctionExecutionJob() noexcept = default;
  FunctionExecutionJob(const std::function<void()>& func, 
                         JobType job_type) noexcept;
  FunctionExecutionJob(FunctionExecutionJob&& other) noexcept = default;
  FunctionExecutionJob& operator=(FunctionExecutionJob&& other) noexcept = default;
  FunctionExecutionJob(const FunctionExecutionJob& other) noexcept = delete;
  FunctionExecutionJob& operator=(
      const FunctionExecutionJob& other) noexcept = delete;
  ~FunctionExecutionJob() noexcept override = default;

  void Work() noexcept override;

private:
  std::function<void()> function_;
};

class LoadFileFromDiskJob final : public Job {
 public:
  LoadFileFromDiskJob() noexcept = default;
  LoadFileFromDiskJob(std::string file_path,
                      FileBuffer* file_buffer,
                      JobType job_type) noexcept;
  LoadFileFromDiskJob(LoadFileFromDiskJob&& other) noexcept = default;
  LoadFileFromDiskJob& operator=(LoadFileFromDiskJob&& other) noexcept = default;
  LoadFileFromDiskJob(const LoadFileFromDiskJob& other) noexcept = delete;
  LoadFileFromDiskJob& operator=(const LoadFileFromDiskJob& other) noexcept =
      delete;
  ~LoadFileFromDiskJob() noexcept override = default;

  void Work() noexcept override;

 private:
  FileBuffer* file_buffer_ = nullptr;
  std::string file_path_{};
};

/*
 * @brief ModelCreationJob is a job that loads the model from the disk and
 * generates the bounding sphere of the mesh.
 */
class ModelCreationJob final : public Job {
public:
  ModelCreationJob() noexcept = default;
  ModelCreationJob(Model* model, std::string_view file_path, bool gamma,
                 bool flip_y) noexcept;
  ModelCreationJob(ModelCreationJob&& other) noexcept = default;
  ModelCreationJob& operator=(ModelCreationJob&& other) noexcept = default;
  ModelCreationJob(const ModelCreationJob& other) noexcept = delete;
  ModelCreationJob& operator=(const ModelCreationJob& other) noexcept = delete;
  ~ModelCreationJob() noexcept override = default;

  void Work() noexcept override;

private:
  Model* model_ = nullptr;
  std::string file_path_{};
  bool gamma_ = false;
  bool flip_y_ = false;
};

class LoadModelToGpuJob final : public Job {
 public:
  LoadModelToGpuJob() = default;
  LoadModelToGpuJob(Model* model) noexcept;
  LoadModelToGpuJob(LoadModelToGpuJob&& other) noexcept = default;
  LoadModelToGpuJob& operator=(LoadModelToGpuJob&& other) noexcept = default;
  LoadModelToGpuJob(const LoadModelToGpuJob& other) noexcept = delete;
  LoadModelToGpuJob& operator=(const LoadModelToGpuJob& other) noexcept =
      delete;
  ~LoadModelToGpuJob() noexcept override = default;

  void Work() noexcept override;

 private:
  Model* model_ = nullptr;
};

class FinalScene final : public Scene {
public:
  void InitOpenGlSettings();
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;
  void DrawImGui() override;

private:
  Renderer renderer_{};

  Camera camera_{};
  Frustum camera_frustum_{};

  glm::mat4 model_ = glm::mat4(1.0f);
  glm::mat4 view_ = glm::mat4(1.0f);
  glm::mat4 projection_ = glm::mat4(1.0f);

  JobSystem job_system_{};

  // Main thread's jobs.
  // -------------------
  FunctionExecutionJob create_framebuffers{};
  FunctionExecutionJob create_meshes_job_{};
  FunctionExecutionJob load_meshes_to_gpu_job_{};
  LoadTextureToGpuJob load_hdr_map_to_gpu_{};
  FunctionExecutionJob init_ibl_maps_job_{};
  LoadModelToGpuJob load_leo_to_gpu_{};
  LoadModelToGpuJob load_sword_to_gpu_{};
  LoadModelToGpuJob load_platform_to_gpu_{};
  LoadModelToGpuJob load_chest_to_gpu_{};
  FunctionExecutionJob set_pipe_tex_units_job_{};
  FunctionExecutionJob create_ssao_data_job_{};
  FunctionExecutionJob apply_shadow_mapping_job_{};
  FunctionExecutionJob init_opengl_settings_job_{};

  std::queue<Job*> main_thread_jobs_{};
  std::vector<LoadTextureToGpuJob> load_tex_to_gpu_jobs_{};
  std::vector<PipelineCreationJob> pipeline_creation_jobs_{};

  // Other thread's jobs.
  // --------------------
  LoadFileFromDiskJob load_hdr_map_{};
  ImageFileDecompressingJob decomp_hdr_map_{};

  ModelCreationJob leo_creation_job_{};
  ModelCreationJob sword_creation_job_{};
  ModelCreationJob platform_creation_job_{};
  ModelCreationJob chest_creation_job_{};

  std::vector<LoadFileFromDiskJob> img_file_loading_jobs_{};
  std::vector<ImageFileDecompressingJob> img_decompressing_jobs_{};
  std::vector<LoadFileFromDiskJob> shader_file_loading_jobs_{};

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
  GLuint shadow_map_fbo_ = 0;
  GLuint shadow_map_ = 0;
  GLuint point_shadow_map_fbo_ = 0;
  GLuint point_shadow_cubemap_ = 0;
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
  bool are_all_data_loaded_ = false;

  // Begin methods.
  // --------------
  void CreatePipelineCreationJobs() noexcept;
  void SetPipelineSamplerTexUnits() noexcept;

  void CreateMeshes() noexcept;
  void LoadMeshesToGpu() noexcept;
  void CreateModelInitializationJobs();
  //void CreateModels() noexcept;
  //void LoadModelsToGpu() noexcept;
  void CreateMaterialsCreationJobs() noexcept;

  void CreateFrameBuffers() noexcept;

  void CreateSsaoData() noexcept;

  void CreateIblMaps() noexcept;
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

  FileBuffer hdr_file_buffer_{};
  ImageBuffer hdr_image_buffer_{};

  static constexpr int shader_count_ = 40;
  static constexpr int pipeline_count_ = shader_count_ / 2;
  static constexpr std::array<std::string_view, shader_count_> shader_paths_{
      "data/shaders/transform/local_transform.vert",
      "data/shaders/hdr/equirectangular_to_cubemap.frag",
      "data/shaders/transform/local_transform.vert",
      "data/shaders/pbr/irradiance_convultion.frag",
      "data/shaders/transform/local_transform.vert",
      "data/shaders/pbr/prefilter.frag",
      "data/shaders/pbr/brdf.vert",
      "data/shaders/pbr/brdf.frag",
      "data/shaders/pbr/pbr_g_buffer.vert",
      "data/shaders/pbr/pbr_g_buffer.frag",
      "data/shaders/pbr/pbr_g_buffer.vert",
      "data/shaders/pbr/arm_pbr_g_buffer.frag",
      "data/shaders/pbr/pbr_g_buffer.vert",
      "data/shaders/pbr/emissive_arm_pbr_g_buffer.frag",
      "data/shaders/pbr/instanced_pbr_g_buffer.vert",
      "data/shaders/pbr/pbr_g_buffer.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/ssao/ssao.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/ssao/ssao_blur.frag",
      "data/shaders/shadow/simple_depth.vert",
      "data/shaders/shadow/simple_depth.frag",
      "data/shaders/shadow/simple_depth.vert",
      "data/shaders/shadow/point_light_simple_depth.frag",
      "data/shaders/shadow/instanced_simple_depth.vert",
      "data/shaders/shadow/simple_depth.frag",
      "data/shaders/shadow/instanced_simple_depth.vert",
      "data/shaders/shadow/point_light_simple_depth.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/pbr/deferred_pbr.frag",
      "data/shaders/transform/transform.vert",
      "data/shaders/visual_debug/light_debug.frag",
      "data/shaders/hdr/hdr_cubemap.vert",
      "data/shaders/hdr/hdr_cubemap.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/bloom/down_sample.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/bloom/up_sample.frag",
      "data/shaders/transform/screen_transform.vert",
      "data/shaders/hdr/hdr.frag",
  };
  std::array<Pipeline*, pipeline_count_> pipelines_{};
  std::array<FileBuffer, shader_count_> shader_file_buffers_{};

  static constexpr std::int8_t texture_count_ = 37;
  std::array<FileBuffer, texture_count_> image_file_buffers_{};
  std::array<ImageBuffer, texture_count_> image_buffers{};
  std::array<TextureParameters, texture_count_> texture_inputs_{};
  std::array<GLuint*, texture_count_> texture_ids_{};
};