#include "final_scene.h"
#include "engine.h"
#include "file_utility.h"

#include <imgui.h>

#ifdef TRACY_ENABLE
#include <TracyC.h>
#include <Tracy.hpp>
#endif  // TRACY_ENABLE

#include <iostream>
#include <random>

void FinalScene::Begin() {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  TextureParameters hdr_map_params("data/textures/hdr/cape_hill_4k.hdr",
                                   GL_CLAMP_TO_EDGE, GL_LINEAR, false, true,
                                   true);

  load_hdr_map_ =
      LoadFileFromDiskJob{hdr_map_params.image_file_path, &hdr_file_buffer_};

  decomp_hdr_map_ =
      ImageFileDecompressingJob{&hdr_file_buffer_, &hdr_image_buffer_,
                                hdr_map_params.flipped_y, hdr_map_params.hdr};
  decomp_hdr_map_.AddDependency(&load_hdr_map_);

  job_system_.AddJob(&load_hdr_map_);
  job_system_.AddJob(&decomp_hdr_map_);


  create_framebuffers = FunctionExecutionJob([this]() { CreateFrameBuffers(); });
  main_thread_jobs_.push(&create_framebuffers);

  // Pipeline jobs.
  // -------------
  CreatePipelineCreationJobs();

  // Meshes initialization jobs.
  // ---------------------------
  create_meshes_job_ = FunctionExecutionJob([this]() { CreateMeshes(); });

  load_meshes_to_gpu_job_ = FunctionExecutionJob(
      [this]() { LoadMeshesToGpu(); });
  load_meshes_to_gpu_job_.AddDependency(&create_meshes_job_);

  job_system_.AddJob(&create_meshes_job_);
  main_thread_jobs_.push(&load_meshes_to_gpu_job_);

  set_pipe_tex_units_job_ = FunctionExecutionJob(
      [this]() { SetPipelineSamplerTexUnits(); });
  main_thread_jobs_.push(&set_pipe_tex_units_job_);
  create_ssao_data_job_ = FunctionExecutionJob([this]() { CreateSsaoData(); });
  main_thread_jobs_.push(&create_ssao_data_job_);

    // Models initialization jobs.
  // ---------------------------
  leo_creation_job_ = ModelCreationJob(
      &leo_magnus_, "data/models/leo_magnus/leo_magnus.obj", true, false);
  sword_creation_job_ = ModelCreationJob(
      &sword_, "data/models/leo_magnus/sword.obj", true, false);
  platform_creation_job_ = ModelCreationJob(
      &sandstone_platform_,
      "data/models/sandstone_platform/sandstone-platform1.obj", true, false);
  chest_creation_job_ = ModelCreationJob(
      &treasure_chest_, "data/models/treasure_chest/treasure_chest_2k.obj",
      true, true);

  load_leo_to_gpu_ = LoadModelToGpuJob(&leo_magnus_);
  load_leo_to_gpu_.AddDependency(&leo_creation_job_);
  load_sword_to_gpu_ = LoadModelToGpuJob(&sword_);
  load_sword_to_gpu_.AddDependency(&sword_creation_job_);
  load_platform_to_gpu_ = LoadModelToGpuJob(&sandstone_platform_);
  load_platform_to_gpu_.AddDependency(&platform_creation_job_);
  load_chest_to_gpu_ = LoadModelToGpuJob(&treasure_chest_);
  load_chest_to_gpu_.AddDependency(&chest_creation_job_);

  job_system_.AddJob(&leo_creation_job_);
  job_system_.AddJob(&sword_creation_job_);
  job_system_.AddJob(&platform_creation_job_);
  job_system_.AddJob(&chest_creation_job_);

  main_thread_jobs_.push(&load_leo_to_gpu_);
  main_thread_jobs_.push(&load_sword_to_gpu_);
  main_thread_jobs_.push(&load_platform_to_gpu_);
  main_thread_jobs_.push(&load_chest_to_gpu_);

      load_hdr_map_to_gpu_ = LoadTextureToGpuJob{
      &hdr_image_buffer_, &equirectangular_map_, hdr_map_params};
  load_hdr_map_to_gpu_.AddDependency(&decomp_hdr_map_);
  main_thread_jobs_.push(&load_hdr_map_to_gpu_);

  init_ibl_maps_job_ = FunctionExecutionJob{[this]() { CreateIblMaps(); }};
  init_ibl_maps_job_.AddDependency(&load_hdr_map_to_gpu_);
  init_ibl_maps_job_.AddDependency(&create_meshes_job_);
  main_thread_jobs_.push(&init_ibl_maps_job_);

  apply_shadow_mapping_job_ = FunctionExecutionJob(
      [this]() { ApplyShadowMappingPass(); });
  main_thread_jobs_.push(&apply_shadow_mapping_job_);

  init_opengl_settings_job_ = FunctionExecutionJob(
      [this]() { InitOpenGlSettings(); });

  main_thread_jobs_.push(&init_opengl_settings_job_);

  CreateMaterialsCreationJobs();

  job_system_.LaunchWorkers(std::thread::hardware_concurrency());
}

void FinalScene::End() {
  DestroyPipelines();

  DestroyMeshes();
  DestroyModels();
  DestroyMaterials();

  DestroyIblPreComputedCubeMaps();

  DestroyFrameBuffers();

  camera_.End();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void FinalScene::Update(float dt) {
  while (!are_all_data_loaded_) {
    if (!main_thread_jobs_.empty()) {
      Job* job = nullptr;
      job = main_thread_jobs_.front();
      if (job->IsReadyToStart()) {
        job->Execute();
        main_thread_jobs_.pop();
      }
      else
      {
        return;
      }
    }
    else {
      job_system_.JoinWorkers();
      are_all_data_loaded_ = true;
      break;
    }
  }

  const auto window_aspect = Engine::window_aspect();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(window_aspect);
  camera_frustum_ = camera_.CalculateFrustum(window_aspect);

  // Draw the geometry and color data in the G-Buffer.
  ApplyGeometryPass();

  // Calculate ambient occlusion based on the G-Buffer data.
  // Draw result in the SSAO frame buffers.
  ApplySsaoPass();

  // Calculate lighting per pixel on deferred shading based on the G-Buffer data.
  // Draw the result in the HDR color buffer.
  ApplyDeferredPbrLightingPass();

  // Draw the objects that are not impacted by light in front shading.
  // Draw the result in the HDR color buffer.
  ApplyFrontShadingPass();

  // Take the most brightness part of the scene to apply a bloom algorithm on it.
  // Draw the result in the bloom frame buffer.
  ApplyBloomPass();

  // Apply tone mapping and gamma correction to the HDR color buffer.
  // Draw the result in the default framebuffer.
  ApplyHdrPass();
}

void FinalScene::OnEvent(const SDL_Event& event) { 
  camera_.OnEvent(event);

  switch (event.type) {
  case SDL_KEYDOWN:
    switch (event.key.keysym.scancode) {
      case SDL_SCANCODE_H:
        is_help_window_open_ = !is_help_window_open_;
        break;
      case SDL_SCANCODE_C:
        break;
      default:
        break;
    }
    break;
  case SDL_WINDOWEVENT: {
    switch (event.window.event) {
      case SDL_WINDOWEVENT_RESIZED: {
        const auto new_size = glm::uvec2(event.window.data1, event.window.data2);
        g_buffer_.Resize(new_size);
        ssao_fbo_.Resize(new_size);
        ssao_blur_fbo_.Resize(new_size);
        hdr_fbo_.Resize(new_size);
        break;
      }
      default:
        break;
    }
    break;
  }
  default:
    break;
  }
}

void FinalScene::DrawImGui() { 
  static const auto window_size = Engine::window_size();

  if (!are_all_data_loaded_)
  {
    
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(window_size.x * 0.4f, window_size.y * 0.35f),
                            ImGuiCond_Once);

    ImGui::Begin("Loading...");

    ImGui::TextWrapped("Loading...");

    ImGui::End();

    return;
  }

  if (!is_help_window_open_) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
  ImGui::SetNextWindowPos(ImVec2(window_size.x * 0.015f, window_size.y * 0.025f),
                          ImGuiCond_Once);

  if (ImGui::Begin("Scene Controls and Settings.", &is_help_window_open_)) {

    if (ImGui::CollapsingHeader("Description.",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::TextWrapped(
          "Welcome to my OpenGL 3D scene.\n"
          "In this window you can find every controls and settings of the "
          "scene.\n"
          "You can open/close this window at any time by pressing the [H] key.\n");
    }

    if (ImGui::CollapsingHeader("Camera Controls.")) {
      ImGui::TextWrapped("Move :");
      ImGui::Indent();
      ImGui::TextWrapped("[W][A][S][D]");
      ImGui::Unindent();

      ImGui::TextWrapped("Rotate and Mouse Relative Mode :");
      ImGui::Indent();
      ImGui::TextWrapped("[Mouse Button Right][ESC]");
      ImGui::Unindent();

      ImGui::TextWrapped("Zoom in/out :");
      ImGui::Indent();
      ImGui::TextWrapped("[Mouse Wheel]");
      ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Light Settings.")) {
      ImGui::Indent();

      if (ImGui::CollapsingHeader("Point Light.")) {
        static auto base_point_light_col = point_lights[0].color;
        static auto base_point_light_pos = point_lights[0].position;
        
        static auto last_position = base_point_light_pos;

        ImGui::Text("Color");
        ImGui::SliderFloat("R##Point", &point_lights[0].color.x, 0.f, 30.f);
        ImGui::SliderFloat("G##Point", &point_lights[0].color.y, 0.f, 30.f);
        ImGui::SliderFloat("B##Point", &point_lights[0].color.z, 0.f, 30.f);
        
        if (ImGui::Button("Reset color##Point", ImVec2(125, 25))) {
          point_lights[0].color = base_point_light_col;
        }

        ImGui::Spacing();

        ImGui::Text("Position");
        ImGui::SliderFloat("X##Point", &point_lights[0].position.x, -10.f, 10.f);
        ImGui::SliderFloat("Y##Point", &point_lights[0].position.y, -10.f, 10.f);
        ImGui::SliderFloat("Z##Point", &point_lights[0].position.z, -10.f, 10.f);

        if (ImGui::Button("Reset position##Point", ImVec2(125, 25))) {
          point_lights[0].position = base_point_light_pos;
        }

        if (point_lights[0].position != last_position) {
          ApplyShadowMappingPass();
        }

        last_position = point_lights[0].position;
      }
      
      if (ImGui::CollapsingHeader("Directional Light.")) {
        static auto base_dir_light_col = dir_light_color_;
        static auto base_dir_light_dir = dir_light_dir_;
        static auto base_dir_light_pos = dir_light_pos_;
        
        static auto last_direction = base_dir_light_dir;
        static auto last_position  = base_dir_light_pos;

        ImGui::Checkbox("Display Debug Sphere", &debug_dir_light_);

        ImGui::Text("Color");
        ImGui::SliderFloat("R##Directional", &dir_light_color_.x, 0.f, 30.f);
        ImGui::SliderFloat("G##Directional", &dir_light_color_.y, 0.f, 30.f);
        ImGui::SliderFloat("B##Directional", &dir_light_color_.z, 0.f, 30.f);
        
        if (ImGui::Button("Reset color##Directional", ImVec2(125, 25))) {
          dir_light_color_ = base_dir_light_col;
        }

        ImGui::Spacing();

        ImGui::Text("Position");
        ImGui::SliderFloat("X##DirectionalPos", &dir_light_pos_.x, -10.f, 10.f);
        ImGui::SliderFloat("Y##DirectionalPos", &dir_light_pos_.y, -10.f, 10.f);
        ImGui::SliderFloat("Z##DirectionalPos", &dir_light_pos_.z, -10.f, 10.f);

        if (ImGui::Button("Reset Position##DirectionalPos", ImVec2(125, 25))) {
          dir_light_pos_ = base_dir_light_pos;
        }

        ImGui::Spacing();

        ImGui::Text("Direction");
        ImGui::SliderFloat("X##Directional", &dir_light_dir_.x, -1.f, 1.f);
        ImGui::SliderFloat("Y##Directional", &dir_light_dir_.y, -1.f, 1.f);
        ImGui::SliderFloat("Z##Directional", &dir_light_dir_.z, -1.f, 1.f);

        if (ImGui::Button("Reset Direction##Directional", ImVec2(125, 25))) {
          dir_light_dir_ = base_dir_light_dir;
        }

        if (last_direction != dir_light_dir_ ||
            last_position != dir_light_pos_) {
          ApplyShadowMappingPass();
        }

        last_direction = dir_light_dir_;
        last_position = dir_light_pos_;
      }

      ImGui::Unindent();
    }

    
    if (ImGui::IsWindowHovered()) {
      camera_.ChangeMouseInputsEnability(false);
    } else {
      camera_.ChangeMouseInputsEnability(true);
    }
  }

  ImGui::End();
}

void FinalScene::InitOpenGlSettings() {
  // Important to call glViewport with the screen dimension after the creation
  // of the different IBL pre-computed textures.
  const auto screen_size = Engine::window_size();
  glViewport(0, 0, screen_size.x, screen_size.y);

  camera_.Begin(glm::vec3(0.0f, -3.75f, 15.0f), 45.f, 0.1f, 100.f, -90.f,
                -10.5f);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void FinalScene::CreatePipelineCreationJobs() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  std::array<Pipeline*, pipeline_count_> pipelines {
    // IBL textures creation pipelines.
    // --------------------------------
    &equirect_to_cubemap_pipe_,
    &irradiance_pipeline_,
    &prefilter_pipeline_,
    &brdf_pipeline_,

    // Geometry pipelines.
    // -------------------
    &geometry_pipeline_, 
    &arm_geometry_pipe_,
    &emissive_arm_geometry_pipe_,
    &instanced_geometry_pipeline_,
    &ssao_pipeline_,
    &ssao_blur_pipeline_,
    &shadow_mapping_pipe_,
    &point_shadow_mapping_pipe_,
    &instanced_shadow_mapping_pipe_,
    &point_instanced_shadow_mapping_pipe_,

    // Drawing and lighting pipelines.
    // -------------------------------
    &pbr_lighting_pipeline_,
    &debug_lights_pipeline_,
    &cubemap_pipeline_,

    //// Postprocessing pipelines.
    //// -------------------------
    &down_sample_pipeline_,
    &up_sample_pipeline_,
    &bloom_hdr_pipeline_,
  };

  pipelines_ = std::move(pipelines);

  shader_file_loading_jobs_.reserve(shader_count_);

  int pipeline_iterator = 0;
  for (int i = 0; i < shader_count_; i++) {
    shader_file_loading_jobs_.emplace_back(LoadFileFromDiskJob(
        shader_paths_[i].data(), 
        &shader_file_buffers_[i]));

    if (i % 2 == 1) {
      pipeline_creation_jobs_[pipeline_iterator] = PipelineCreationJob(
          &shader_file_buffers_[i - 1], &shader_file_buffers_[i],
          pipelines_[pipeline_iterator]);

      pipeline_creation_jobs_[pipeline_iterator].AddDependency(&shader_file_loading_jobs_[i - 1]);
      pipeline_creation_jobs_[pipeline_iterator].AddDependency(&shader_file_loading_jobs_[i]);

      pipeline_iterator++;
    }
  }

  for (auto& shader_loading_job : shader_file_loading_jobs_) {
    job_system_.AddJob(&shader_loading_job);
  }

  for (auto& prog_creation_job : pipeline_creation_jobs_) {
    main_thread_jobs_.push(&prog_creation_job);
  }
}

void FinalScene::SetPipelineSamplerTexUnits() noexcept {
  // Setup the sampler2D uniforms.
  // -----------------------------
  geometry_pipeline_.Bind();
  geometry_pipeline_.SetInt("material.albedo_map", 0);
  geometry_pipeline_.SetInt("material.normal_map", 1);
  geometry_pipeline_.SetInt("material.metallic_map", 2);
  geometry_pipeline_.SetInt("material.roughness_map", 3);
  geometry_pipeline_.SetInt("material.ao_map", 4);

  arm_geometry_pipe_.Bind();
  arm_geometry_pipe_.SetInt("material.albedo_map", 0);
  arm_geometry_pipe_.SetInt("material.normal_map", 1);
  arm_geometry_pipe_.SetInt("material.ao_metallic_roughness_map", 2);

  emissive_arm_geometry_pipe_.Bind();
  emissive_arm_geometry_pipe_.SetInt("material.albedo_map", 0);
  emissive_arm_geometry_pipe_.SetInt("material.normal_map", 1);
  emissive_arm_geometry_pipe_.SetInt("material.ao_metallic_roughness_map", 2);
  emissive_arm_geometry_pipe_.SetInt("material.emissive_map", 3);

  instanced_geometry_pipeline_.Bind();
  instanced_geometry_pipeline_.SetInt("material.albedo_map", 0);
  instanced_geometry_pipeline_.SetInt("material.normal_map", 1);
  instanced_geometry_pipeline_.SetInt("material.metallic_map", 2);
  instanced_geometry_pipeline_.SetInt("material.roughness_map", 3);
  instanced_geometry_pipeline_.SetInt("material.ao_map", 4);

  ssao_pipeline_.Bind();
  ssao_pipeline_.SetInt("gViewPositionMetallic", 0);
  ssao_pipeline_.SetInt("gViewNormalRoughness", 1);
  ssao_pipeline_.SetInt("texNoise", 2);
  ssao_pipeline_.SetFloat("radius", kSsaoRadius);
  ssao_pipeline_.SetFloat("biais", kSsaoBiais);

  ssao_blur_pipeline_.Bind();
  ssao_blur_pipeline_.SetInt("ssaoInput", 0);

  pbr_lighting_pipeline_.Bind();
  pbr_lighting_pipeline_.SetInt("irradianceMap", 0);
  pbr_lighting_pipeline_.SetInt("prefilterMap", 1);
  pbr_lighting_pipeline_.SetInt("brdfLUT", 2);
  pbr_lighting_pipeline_.SetInt("gViewPositionMetallic", 3);
  pbr_lighting_pipeline_.SetInt("gViewNormalRoughness", 4);
  pbr_lighting_pipeline_.SetInt("gAlbedoAmbientOcclusion", 5);
  pbr_lighting_pipeline_.SetInt("gEmissive", 6);
  pbr_lighting_pipeline_.SetInt("ssao", 7);
  pbr_lighting_pipeline_.SetInt("shadowMap", 8);
  pbr_lighting_pipeline_.SetInt("shadowCubeMap", 9);
  pbr_lighting_pipeline_.SetFloat("combined_ao_factor", kCombiendAoFactor);
  pbr_lighting_pipeline_.SetFloat("emissive_factor", 15.f);

  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetInt("environmentMap", 0);

  bloom_hdr_pipeline_.Bind();
  bloom_hdr_pipeline_.SetInt("hdrBuffer", 0);
  bloom_hdr_pipeline_.SetInt("bloomBlur", 1);
}

void FinalScene::CreateIblMaps() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  CreateHdrCubemap();
  CreateIrradianceCubeMap();
  CreatePrefilterCubeMap();
  CreateBrdfLut();
}

void FinalScene::CreateHdrCubemap() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

    const auto screen_size = Engine::window_size();

    //const auto equirectangular_map = LoadHDR_Texture("data/textures/hdr/cape_hill_4k.hdr",
    //                                                 GL_CLAMP_TO_EDGE, GL_LINEAR);

    glGenTextures(1, &env_cubemap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    for (GLuint i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                 kSkyboxResolution, kSkyboxResolution, 0, GL_RGB, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirect_to_cubemap_pipe_.Bind();
    equirect_to_cubemap_pipe_.SetInt("equirectangularMap", 0);
    equirect_to_cubemap_pipe_.SetMatrix4("transform.projection",
                                         capture_projection_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, equirectangular_map_);

    capture_fbo_.Bind();
    capture_fbo_.Resize(glm::uvec2(kSkyboxResolution));
    glViewport(0, 0, kSkyboxResolution, kSkyboxResolution);


    for (GLuint i = 0; i < 6; i++) {
      equirect_to_cubemap_pipe_.SetMatrix4("transform.view", capture_views_[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env_cubemap_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      renderer_.DrawMesh(cubemap_mesh_);
    }

    capture_fbo_.UnBind();

    // Then generate mipmaps.
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    //glDeleteTextures(1, &equirectangular_map_);
}

void FinalScene::CreateIrradianceCubeMap() noexcept {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif  // TRACY_ENABLE

  glGenTextures(1, &irradiance_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);

  for (GLuint i = 0; i < 6; i++) {
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
              kIrradianceMapResolution, kIrradianceMapResolution, 0,
              GL_RGB, GL_FLOAT, NULL);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  capture_fbo_.Bind();
  capture_fbo_.Resize(glm::uvec2(kIrradianceMapResolution));

  irradiance_pipeline_.Bind();
  irradiance_pipeline_.SetInt("environmentMap", 0);
  irradiance_pipeline_.SetMatrix4("transform.projection", capture_projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

  glViewport(0, 0, kIrradianceMapResolution, kIrradianceMapResolution);  
  capture_fbo_.Bind();

  for (GLuint i = 0; i < 6; i++) {
    irradiance_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                           irradiance_cubemap_, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    renderer_.DrawMesh(cubemap_mesh_);
  }

  capture_fbo_.UnBind();
}

void FinalScene::CreatePrefilterCubeMap() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  glGenTextures(1, &prefilter_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);

  for (GLuint i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                   kPrefilterMapResolution, kPrefilterMapResolution, 0, GL_RGB,
                   GL_FLOAT, NULL);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  prefilter_pipeline_.Bind();
  prefilter_pipeline_.SetInt("environmentMap", 0);
  prefilter_pipeline_.SetMatrix4("transform.projection", capture_projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);

  capture_fbo_.Bind();

  GLuint maxMipLevels = 5;
  for (GLuint mip = 0; mip < maxMipLevels; mip++) {
      // reisze framebuffer according to mip-level size.
      GLuint mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
      GLuint mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));

      capture_fbo_.Resize(glm::uvec2(mipWidth, mipHeight));
      glViewport(0, 0, mipWidth, mipHeight);

      float roughness = (float)mip / (float)(maxMipLevels - 1);
      prefilter_pipeline_.SetFloat("roughness", roughness);

      for (GLuint i = 0; i < 6; i++) {
        prefilter_pipeline_.SetMatrix4("transform.view", capture_views_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_cubemap_,
                               mip);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderer_.DrawMesh(cubemap_mesh_);
      }
  }

  capture_fbo_.UnBind();
}

void FinalScene::CreateBrdfLut() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  glGenTextures(1, &brdf_lut_);

  // pre-allocate enough memory for the LUT texture.
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kBrdfLutResolution,
               kBrdfLutResolution, 0, GL_RG, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  capture_fbo_.Bind();
  capture_fbo_.Resize(glm::uvec2(kBrdfLutResolution));
  glViewport(0, 0, kBrdfLutResolution, kBrdfLutResolution);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         brdf_lut_, 0);

  brdf_pipeline_.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
  renderer_.DrawMesh(screen_quad_);

  capture_fbo_.UnBind();
}

void FinalScene::CreateFrameBuffers() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  const auto screen_size = Engine::window_size();

  // IBL capture texture data framebuffer.
  // -------------------------------------
  DepthStencilAttachment capture_depth_stencil_attach(GL_DEPTH_COMPONENT24,
                                                      GL_DEPTH_ATTACHMENT);
  FrameBufferSpecification capture_specification;
  capture_specification.SetSize(glm::uvec2(kSkyboxResolution, kSkyboxResolution));
  capture_specification.SetDepthStencilAttachment(capture_depth_stencil_attach);
  // Color attachments are apart of the framebuffer in order to send them
  // easily to the pbr shaders.
  capture_fbo_.Create(capture_specification);
  capture_fbo_.Bind();
  // Configure a basic color attachment.
  constexpr GLenum buf = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(static_cast<GLsizei>(1), &buf);
  capture_fbo_.UnBind();

  // Configure G-Buffer Framebuffer.
  // -------------------------------
  ColorAttachment g_pos_metallic_attachment(GL_RGBA16F, GL_RGBA, GL_NEAREST,
                                            GL_CLAMP_TO_EDGE);
  ColorAttachment g_normal_roughness_attachment(GL_RGBA16F, GL_RGBA, GL_NEAREST,
                                                GL_CLAMP_TO_EDGE);
  ColorAttachment g_albedo_ao_attachment(GL_RGBA, GL_RGBA, GL_NEAREST,
                                         GL_CLAMP_TO_EDGE);
  ColorAttachment g_emissive(GL_RGB, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE);

  DepthStencilAttachment g_depth_stencil_attachment(GL_DEPTH_COMPONENT24,
                                                    GL_DEPTH_ATTACHMENT);

  FrameBufferSpecification g_buffer_specification;
  g_buffer_specification.SetSize(screen_size);
  g_buffer_specification.PushColorAttachment(g_pos_metallic_attachment);
  g_buffer_specification.PushColorAttachment(g_normal_roughness_attachment);
  g_buffer_specification.PushColorAttachment(g_albedo_ao_attachment);
  g_buffer_specification.PushColorAttachment(g_emissive);
  g_buffer_specification.SetDepthStencilAttachment(g_depth_stencil_attachment);

  g_buffer_.Create(g_buffer_specification);

  // SSAO framebuffers.
  // ------------------
  ColorAttachment ssao_color_attach(GL_RED, GL_RED, GL_NEAREST,
                                    GL_CLAMP_TO_EDGE);
  FrameBufferSpecification ssao_specification;
  ssao_specification.SetSize(screen_size);
  ssao_specification.PushColorAttachment(ssao_color_attach);

  ssao_fbo_.Create(ssao_specification);

  ColorAttachment ssao_blur_color_attach(GL_RED, GL_RED, GL_NEAREST,
                                         GL_CLAMP_TO_EDGE);
  FrameBufferSpecification ssao_blur_specification;
  ssao_blur_specification.SetSize(screen_size);
  ssao_blur_specification.PushColorAttachment(ssao_blur_color_attach);

  ssao_blur_fbo_.Create(ssao_blur_specification);

  // Shadow map Framebuffer.
  // -----------------------
  glGenFramebuffers(1, &shadow_map_fbo_);
  // create depth texture
  glGenTextures(1, &shadow_map_);
  glBindTexture(GL_TEXTURE_2D, shadow_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapWidth_,
               kShadowMapHeight_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  constexpr std::array<float, 4> border_colors_ = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                   border_colors_.data());

  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadow_map_, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Point Shadow Cubemap Framebuffer.
  // ---------------------------------
  glGenFramebuffers(1, &point_shadow_map_fbo_);
  // create depth cubemap texture
  glGenTextures(1, &point_shadow_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, point_shadow_cubemap_);
  for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
                   kPointShadowMapRes, kPointShadowMapRes, 0,
                   GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, point_shadow_map_fbo_);
  GLenum drawBuffers[] = { GL_NONE };
  glDrawBuffers(1, drawBuffers);
  glReadBuffer(GL_NONE);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X, point_shadow_cubemap_, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Bloom Framebuffer.
  // ------------------
  bool status = bloom_fbo_.Init(screen_size.x, screen_size.y, kBloomMipsCount_);
  if (!status) {
      std::cerr << "Failed to initialize bloom FBO - cannot create bloom renderer!\n";
  }

  // HDR framebuffer.
  // ----------------

  // 1st attachment stores all pixels on screen.
  ColorAttachment hdr_color_attachment(GL_RGBA16F, GL_RGBA, GL_LINEAR,
                                       GL_CLAMP_TO_EDGE);
  // 2nd attachment stores all bright pixels (pixels with a value greater than 1.0).
  ColorAttachment bright_color_attachment(GL_RGBA16F, GL_RGBA, GL_LINEAR,
                                          GL_CLAMP_TO_EDGE);
  DepthStencilAttachment hdr_depth_stencil_attachment(GL_DEPTH_COMPONENT24,
                                                      GL_DEPTH_ATTACHMENT);
  FrameBufferSpecification hdr_specification;
  hdr_specification.SetSize(screen_size);
  hdr_specification.PushColorAttachment(hdr_color_attachment);
  hdr_specification.PushColorAttachment(bright_color_attachment);
  hdr_specification.SetDepthStencilAttachment(hdr_depth_stencil_attachment);

  hdr_fbo_.Create(hdr_specification);
}

void FinalScene::CreateSsaoData() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  // generate sample kernel
  // ----------------------
  std::uniform_real_distribution<GLfloat> random_floats(0.0, 1.0);
  // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;

  for (GLuint i = 0; i < kSsaoKernelSampleCount_; i++) {
      glm::vec3 sample(random_floats(generator) * 2.0 - 1.0,
                       random_floats(generator) * 2.0 - 1.0,
                       random_floats(generator));
      sample = glm::normalize(sample);
      sample *= random_floats(generator);
      float scale = float(i) / kSsaoKernelSampleCount_;

      scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
      sample *= scale;
      ssao_kernel_[i] = sample;
  }

  static constexpr auto kSsaoNoiseDimensionXY =
      kSsaoNoiseDimensionX_ * kSsaoNoiseDimensionY_;

  std::array<glm::vec3, kSsaoNoiseDimensionXY> ssao_noise{};
  for (GLuint i = 0; i < kSsaoNoiseDimensionXY; i++) {
      // As the sample kernel is oriented along the positive z direction in
      // tangent space, we leave the z component at 0.0 so we rotate around the
      // z axis.
      glm::vec3 noise(random_floats(generator) * 2.0 - 1.0,
                      random_floats(generator) * 2.0 - 1.0, 0.0f);
      ssao_noise[i] = noise;
  }

  glGenTextures(1, &noise_texture_);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kSsaoNoiseDimensionX_,
               kSsaoNoiseDimensionX_, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void FinalScene::CreateMeshes() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  sphere_.CreateSphere();
  // Generate bounding sphere volume to test intersection with the camera frustum.
  sphere_.GenerateBoundingSphere(); 

  cubemap_mesh_.CreateCubeMap();

  screen_quad_.CreateScreenQuad();
}

void FinalScene::LoadMeshesToGpu() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  sphere_.LoadToGpu();

  // Load sphere model matrices to the GPU.
  // --------------------------------------
  sphere_model_matrices_.reserve(kSphereCount_);
  visible_sphere_model_matrices_.reserve(kSphereCount_);

  model_ = glm::mat4(1.f);
  model_ =
      glm::translate(model_, treasure_chest_pos_ + glm::vec3(3.f, 0.6f, -3.5f));
  model_ = glm::scale(model_, glm::vec3(0.75f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ =
      glm::translate(model_, treasure_chest_pos_ + glm::vec3(-3.f, 0.55f, 3.f));
  model_ = glm::scale(model_, glm::vec3(0.55f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(1.5f, 0.6f, 1.75f));
  model_ = glm::scale(model_, glm::vec3(0.6f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(-1.f, 0.475f, 2.25f));
  model_ = glm::scale(model_, glm::vec3(0.45f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(0.3f, 0.25f, 1.75f));
  model_ = glm::scale(model_, glm::vec3(0.2f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(2.75f, 0.45f, -0.5f));
  model_ = glm::scale(model_, glm::vec3(0.5f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(-2.75f, 0.8f, 1.f));
  model_ = glm::scale(model_, glm::vec3(0.8f));
  sphere_model_matrices_.push_back(model_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_,
                          treasure_chest_pos_ + glm::vec3(-2.f, 0.85f, -2.75f));
  model_ = glm::scale(model_, glm::vec3(1.f));
  sphere_model_matrices_.push_back(model_);

  sphere_.SetupModelMatrixBuffer(sphere_model_matrices_.data(),
                                 sphere_model_matrices_.size(),
                                 GL_DYNAMIC_DRAW);

  cubemap_mesh_.LoadToGpu();
  screen_quad_.LoadToGpu();
}

void FinalScene::CreateMaterialsCreationJobs() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  std::array<TextureParameters, texture_count_> texture_inputs{
    // Gold Material.
    // --------------
    TextureParameters("data/textures/pbr/gold/gold-scuffed_basecolor-boosted.png", 
    GL_CLAMP_TO_EDGE, GL_LINEAR, true, false),
    TextureParameters("data/textures/pbr/gold/gold-scuffed_normal.png", 
    GL_CLAMP_TO_EDGE, GL_LINEAR, false, false),
    TextureParameters("data/textures/pbr/gold/gold-scuffed_metallic.png", 
    GL_CLAMP_TO_EDGE, GL_LINEAR, false, false),
    TextureParameters("data/textures/pbr/gold/gold-scuffed_roughness.png", 
    GL_CLAMP_TO_EDGE, GL_LINEAR, false, false),
    TextureParameters("data/textures/pbr/gold/ao.png", 
    GL_CLAMP_TO_EDGE, GL_LINEAR, false, false),

    // Leo Magnus' textures.
    // ---------------------
    // Grosse armure.
    // --------------
    TextureParameters("data/models/leo_magnus/leo_magnus_low_grosse_armure_BaseColor.png", GL_REPEAT, GL_LINEAR, true, true),
    TextureParameters("data/models/leo_magnus/leo_magnus_low_grosse_armure_Normal.png", GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_grosse_armure_OcclusionRoughnessMetallic.png",
          GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/no_emissive.jpg", GL_REPEAT,
                        GL_LINEAR, false, true),
    // Cape.
    // -----
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_cape_BaseColor.png", GL_REPEAT,
          GL_LINEAR, true, true),
    TextureParameters("data/models/leo_magnus/leo_magnus_low_cape_Normal.png",
                        GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/"
                        "leo_magnus_low_cape_OcclusionRoughnessMetallic.png",
                        GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/no_emissive.jpg", GL_REPEAT,
                        GL_LINEAR, false, true),
    // Tete.
    // -----
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_tete_BaseColor.png", GL_REPEAT,
          GL_LINEAR, true, true),
    TextureParameters("data/models/leo_magnus/leo_magnus_low_tete_Normal.png",
                        GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/"
                        "leo_magnus_low_tete_OcclusionRoughnessMetallic.png",
                        GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_tete_Emissive.png", GL_REPEAT,
          GL_LINEAR, true, true),
    // Pilosite.
    // --------------
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_pilosite_BaseColor.png",
          GL_REPEAT, GL_LINEAR, true, true),
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_pilosite_Normal.png",
          GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/"
          "leo_magnus_low_pilosite_OcclusionRoughnessMetallic.png",
          GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/no_emissive.jpg", GL_REPEAT,
                        GL_LINEAR, false, true),
    // Petite armure.
    // --------------
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_petite_armure_BaseColor.png",
          GL_REPEAT, GL_LINEAR, true, true),
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_petite_armure_Normal.png",
          GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/"
          "leo_magnus_low_petite_armure_OcclusionRoughnessMetallic.png",
          GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters(
          "data/models/leo_magnus/leo_magnus_low_petite_armure_Emissive.png",
          GL_REPEAT, GL_LINEAR, true, true),

    // Sword textures.
    // ---------------
    TextureParameters("data/models/leo_magnus/epee_low_1001_BaseColor.png",
      GL_REPEAT, GL_LINEAR, true, true),
    TextureParameters("data/models/leo_magnus/epee_low_1001_Normal.png",
      GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/epee_low_1001_OcclusionRoughnessMetallic.png", 
      GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/leo_magnus/epee_low_1001_Emissive.png", 
      GL_REPEAT, GL_LINEAR, true, true),

    // Sandstone platform textures.
    // ----------------------------
    TextureParameters("data/models/sandstone_platform/sandstone-platform1-albedo.png", 
      GL_REPEAT, GL_LINEAR, true, true),
    TextureParameters("data/models/sandstone_platform/sandstone-platform1-normal_ogl.png", 
      GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/sandstone_platform/sandstone-platform1-metallic.png", 
      GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/sandstone_platform/sandstone-platform1-roughness.png", 
      GL_REPEAT, GL_LINEAR, false, true),
    TextureParameters("data/models/sandstone_platform/sandstone-platform1-ao.png", 
      GL_REPEAT, GL_LINEAR, false, true),

    // Treasure chest textures.
    // ------------------------
    TextureParameters("data/models/treasure_chest/treasure_chest_diff_2k.jpg",
      GL_REPEAT, GL_LINEAR, true, false),
    TextureParameters("data/models/treasure_chest/treasure_chest_nor_gl_2k.jpg",
      GL_REPEAT, GL_LINEAR, false, false),
    TextureParameters("data/models/treasure_chest/treasure_chest_arm_2k.jpg",
      GL_REPEAT, GL_LINEAR, false, false),
  };

  std::array<GLuint*, texture_count_> texture_ids { 
    &gold_mat_.albedo_map,
    &gold_mat_.normal_map,
    &gold_mat_.metallic_map,
    &gold_mat_.roughness_map,
    &gold_mat_.ao_map,
  };

  leo_magnus_textures_.resize(20, 0);
  for (int i = 0; i < leo_magnus_textures_.size(); i++) {
    texture_ids[i + 5] = &leo_magnus_textures_[i];
  }

  sword_textures_.resize(4, 0);
  for (int i = 0; i < sword_textures_.size(); i++) {
    texture_ids[i + 25] = &sword_textures_[i];
  }

  texture_ids[29] = &sandstone_platform_mat_.albedo_map;
  texture_ids[30] = &sandstone_platform_mat_.normal_map;
  texture_ids[31] = &sandstone_platform_mat_.metallic_map;
  texture_ids[32] = &sandstone_platform_mat_.roughness_map;
  texture_ids[33] = &sandstone_platform_mat_.ao_map;

  treasure_chest_textures_.resize(3, 0);
  for (int i = 0; i < treasure_chest_textures_.size(); i++) {
    texture_ids[i + 34] = &treasure_chest_textures_[i];
  }

  texture_ids_ = std::move(texture_ids);

  // For loop that creates all the jobs used to create textures for materials.
  for (std::int8_t i = 0; i < texture_count_; i++) {
    const auto& tex_param = texture_inputs[i];

    // Image files reading job.
    // ------------------------
    img_file_loading_jobs_[i] = LoadFileFromDiskJob(
        tex_param.image_file_path, &image_file_buffers_[i]);

    // Image files decompressing job.
    // ------------------------------
    img_decompressing_jobs_[i] = ImageFileDecompressingJob(&image_file_buffers_[i], &image_buffers[i],
                                        tex_param.flipped_y, tex_param.hdr);

    img_decompressing_jobs_[i].AddDependency(&img_file_loading_jobs_[i]);
    img_decompressing_jobs_[i].AddDependency(&decomp_hdr_map_);

    // Texture loading to GPU job.
    // ---------------------------
    load_tex_to_gpu_jobs_[i] = LoadTextureToGpuJob(&image_buffers[i], 
        texture_ids[i], tex_param);

    load_tex_to_gpu_jobs_[i].AddDependency(&img_decompressing_jobs_[i]);
  }

  for (auto& reading_job : img_file_loading_jobs_) {
      job_system_.AddJob(&reading_job);
  }

  decompress_all_images_job_ = DecompressAllImagesJob(&img_decompressing_jobs_);
  job_system_.AddJob(&decompress_all_images_job_);

  for (auto& load_tex_to_gpu : load_tex_to_gpu_jobs_) {
    main_thread_jobs_.push(&load_tex_to_gpu);
  }
}

void FinalScene::ApplyGeometryPass() noexcept {
  // Render all geometric/color data to g-buffer
  g_buffer_.Bind();
  glClearColor(0.0, 0.0, 0.0, 1.0); // keep it black so it doesn't leak into g-buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Draw instanced meshes.
  // ----------------------
  instanced_geometry_pipeline_.Bind();

  instanced_geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  instanced_geometry_pipeline_.SetMatrix4("transform.view", view_);

  DrawInstancedObjectGeometry(GeometryPipelineType::kGeometry);

  // Draw single meshes.
  // -------------------
  geometry_pipeline_.Bind();

  geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  geometry_pipeline_.SetMatrix4("transform.view", view_);

  DrawObjectGeometry(GeometryPipelineType::kGeometry);



  g_buffer_.UnBind();
}

void FinalScene::ApplySsaoPass() noexcept {
  const auto screen_size = Engine::window_size();
  // SSAO texture creation.
  // ----------------------
  ssao_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_CULL_FACE);
  ssao_pipeline_.Bind();
  // Send kernel + rotation
  for (unsigned int i = 0; i < kSsaoKernelSampleCount_; ++i) {
      ssao_pipeline_.SetVec3("samples[" + std::to_string(i) + "]",
                             ssao_kernel_[i]);
  }

  ssao_pipeline_.SetMatrix4("projection", projection_);
  ssao_pipeline_.SetVec2("noiseScale", glm::vec2(screen_size.x / kSsaoNoiseDimensionX_, 
                                                 screen_size.y / kSsaoNoiseDimensionY_));

  glActiveTexture(GL_TEXTURE0);
  g_buffer_.BindColorBuffer(0);  // position-metallic map.
  glActiveTexture(GL_TEXTURE1);
  g_buffer_.BindColorBuffer(1);  // normal-roughness map.
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);

  renderer_.DrawMesh(screen_quad_);

  // SSAO blur.
  // ----------
  ssao_blur_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT);
  ssao_blur_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  ssao_fbo_.BindColorBuffer(0);

  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::ApplyShadowMappingPass() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  // Directional light shadow mapping.
  // ---------------------------------

  // Render scene from light's point of view.
  glViewport(0, 0, kShadowMapWidth_, kShadowMapHeight_);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo_);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  //dir_light_dir_ = glm::normalize(glm::vec3(0) - dir_light_pos_);

  glm::mat4 light_projection, light_view;
  float near_plane = 5.0f, far_plane = 35;
  float width = 20.f, height = 20.f;
  light_projection = glm::ortho(-width, width, -height, height, near_plane, far_plane);
  light_view = glm::lookAt(dir_light_pos_, dir_light_pos_ + dir_light_dir_, 
      glm::vec3(0.0, 1.0, 0.0));
  light_space_matrix_ = light_projection * light_view;

  // Draw instanced meshes.
  // ----------------------
  instanced_shadow_mapping_pipe_.Bind();
  instanced_shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix",
                                            light_space_matrix_);

  DrawInstancedObjectGeometry(GeometryPipelineType::kShadowMapping);

  // Draw single meshes.
  // -------------------
  shadow_mapping_pipe_.Bind();
  shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  DrawObjectGeometry(GeometryPipelineType::kShadowMapping);

  // Point light shadow mapping.
  // ---------------------------
  glViewport(0, 0, kPointShadowMapRes, kPointShadowMapRes);
  glBindFramebuffer(GL_FRAMEBUFFER, point_shadow_map_fbo_);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  static constexpr float aspect = (float)kPointShadowMapRes / (float)kPointShadowMapRes;

  point_instanced_shadow_mapping_pipe_.Bind();

  point_instanced_shadow_mapping_pipe_.SetVec3("light_pos",
                                               point_lights[0].position);

  for (int i = 0; i < 6; i++) {
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, point_shadow_cubemap_, 0);
    glClear(GL_DEPTH_BUFFER_BIT);

    light_view = glm::lookAt(point_lights[0].position,
                            point_lights[0].position + light_dirs_[i], light_ups_[i]);

    light_projection = glm::perspective(glm::radians(90.f), aspect,
                                       kLightNearPlane, kLightFarPlane);

    point_light_space_matrix_ = light_projection * light_view;

    point_instanced_shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix",
                                                    point_light_space_matrix_);
    point_instanced_shadow_mapping_pipe_.SetFloat("light_far_plane", kLightFarPlane);

    DrawInstancedObjectGeometry(GeometryPipelineType::kShadowMapping);
  }

  point_shadow_mapping_pipe_.Bind();

  point_shadow_mapping_pipe_.SetVec3("light_pos",
                                     point_lights[0].position);

  for (int i = 0; i < 6; i++) {
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                           point_shadow_cubemap_, 0);
    light_view =
        glm::lookAt(point_lights[0].position,
                    point_lights[0].position + light_dirs_[i], light_ups_[i]);

    light_projection = glm::perspective(glm::radians(90.f), aspect,
                         kLightNearPlane, kLightFarPlane);

    point_light_space_matrix_ = light_projection * light_view;

    point_shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix",
                                           point_light_space_matrix_);
    point_shadow_mapping_pipe_.SetFloat("light_far_plane",
                                        kLightFarPlane);
    // simpleDepthShader_.SetVec3("lightDir", lightDirs[i]);

    DrawObjectGeometry(GeometryPipelineType::kPointShadowMapping);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  const auto window_size = Engine::window_size();
  glViewport(0, 0, window_size.x, window_size.y);
}

void FinalScene::ApplyDeferredPbrLightingPass() noexcept {
  // Draw the scene in the hdr framebuffer with PBR lighting.
  // --------------------------------------------------------
  hdr_fbo_.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  pbr_lighting_pipeline_.Bind();

  pbr_lighting_pipeline_.SetVec3("viewPos", camera_.position());
  pbr_lighting_pipeline_.SetMatrix4("inverseViewMatrix", glm::inverse(view_));

  // Bind pre-computed IBL data.
  // ---------------------------
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, brdf_lut_);

  glActiveTexture(GL_TEXTURE3);
  g_buffer_.BindColorBuffer(0); // position-metallic map.
  glActiveTexture(GL_TEXTURE4);
  g_buffer_.BindColorBuffer(1); // normal-roughness map.
  glActiveTexture(GL_TEXTURE5);
  g_buffer_.BindColorBuffer(2); // albedo-ao map.
  glActiveTexture(GL_TEXTURE6);
  g_buffer_.BindColorBuffer(3); // emissive map.
  glActiveTexture(GL_TEXTURE7);
  ssao_blur_fbo_.BindColorBuffer(0); // blured ssao map.

  glActiveTexture(GL_TEXTURE8);
  glBindTexture(GL_TEXTURE_2D, shadow_map_);

  glActiveTexture(GL_TEXTURE9);
  glBindTexture(GL_TEXTURE_CUBE_MAP, point_shadow_cubemap_);


  // Set direction light uniforms.
  // -----------------------------
  const auto light_view_dir = glm::vec3(view_ * glm::vec4(dir_light_dir_, 1.0f));
  pbr_lighting_pipeline_.SetVec3("directional_light.world_direction", dir_light_dir_);
  pbr_lighting_pipeline_.SetVec3("directional_light.color", dir_light_color_);
  pbr_lighting_pipeline_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  // Set point lights unfiorms.
  // --------------------------
  for (unsigned int i = 0; i < kLightCount; i++) {
    // Transform light positions in view space because of the SSAO which 
    // needs view space.
    //const auto light_view_pos = glm::vec3(view_ * glm::vec4(light_positions_[i], 1.0));

    pbr_lighting_pipeline_.SetFloat("light_far_plane", kLightFarPlane);

    pbr_lighting_pipeline_.SetVec3("point_lights[" + std::to_string(i) + "].position", 
                                    point_lights[i].position);
    pbr_lighting_pipeline_.SetVec3("point_lights[" + std::to_string(i) + "].color", 
                                    point_lights[i].color);
    pbr_lighting_pipeline_.SetFloat("point_lights[" + std::to_string(i) + "].constant", 
                                    point_lights[i].constant);
    pbr_lighting_pipeline_.SetFloat("point_lights[" + std::to_string(i) + "].linear", 
                                    point_lights[i].linear);
    pbr_lighting_pipeline_.SetFloat("point_lights[" + std::to_string(i) + "].quadratic", 
                                    point_lights[i].quadratic);
  }

  glDisable(GL_CULL_FACE);
  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::ApplyFrontShadingPass() noexcept {
  const auto screen_size = Engine::window_size();

  g_buffer_.BindRead();
  hdr_fbo_.BindDraw();
  glBlitFramebuffer(0, 0, screen_size.x, screen_size.y, 0, 0, screen_size.x,
                    screen_size.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  hdr_fbo_.Bind();

  debug_lights_pipeline_.Bind();

  debug_lights_pipeline_.SetMatrix4("transform.view", view_);
  debug_lights_pipeline_.SetMatrix4("transform.projection", projection_);

  for (unsigned int i = 0; i < kLightCount; i++) {
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, point_lights[i].position);
    model_ = glm::scale(model_, glm::vec3(0.35f));
    debug_lights_pipeline_.SetMatrix4("transform.model", model_);

    debug_lights_pipeline_.SetVec3("lightColor", point_lights[i].color);

    renderer_.DrawMesh(sphere_, GL_TRIANGLE_STRIP);
  }

  if (debug_dir_light_) {
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, dir_light_pos_);
    model_ = glm::scale(model_, glm::vec3(0.35f));
    debug_lights_pipeline_.SetMatrix4("transform.model", model_);

    debug_lights_pipeline_.SetVec3("lightColor", dir_light_color_);

    renderer_.DrawMesh(sphere_, GL_TRIANGLE_STRIP);
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);  // Cull skybox front faces because we are inside the cubemap.

  // Render skybox as last to prevent overdraw.
  cubemap_pipeline_.Bind();
  cubemap_pipeline_.SetMatrix4("transform.view", view_);
  cubemap_pipeline_.SetMatrix4("transform.projection", projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap_);
  renderer_.DrawMesh(cubemap_mesh_);

  hdr_fbo_.UnBind();
}

void FinalScene::ApplyBloomPass() noexcept {
  const auto window_size = Engine::window_size();

  bloom_fbo_.Bind();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  down_sample_pipeline_.Bind();

  const auto& mip_chain = bloom_fbo_.mip_chain();

  down_sample_pipeline_.SetVec2("srcResolution", window_size);

  // Bind srcTexture (HDR color buffer) as initial texture input
  glActiveTexture(GL_TEXTURE0);
  hdr_fbo_.BindColorBuffer(1);

  // Progressively downsample through the mip chain.
  for (int i = 0; i < mip_chain.size(); i++) {
    const BloomMip& mip = mip_chain[i];

    glViewport(0, 0, mip.size.x, mip.size.y);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           mip.texture, 0);

    // Render screen-filled quad of resolution of current mip
    renderer_.DrawMesh(screen_quad_);

    // Set current mip resolution as srcResolution for next iteration
    down_sample_pipeline_.SetVec2("srcResolution", mip.size);
    // Set current mip as texture input for next iteration
    glBindTexture(GL_TEXTURE_2D, mip.texture);
  }

  up_sample_pipeline_.Bind();
  up_sample_pipeline_.SetFloat("filterRadius", kbloomFilterRadius_);

  // Enable additive blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

  for (int i = mip_chain.size() - 1; i > 0; i--) {
    const BloomMip& mip = mip_chain[i];
    const BloomMip& nextMip = mip_chain[i - 1];

    // Bind viewport and texture from where to read
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mip.texture);

    // Set framebuffer render target (we write to this texture)
    glViewport(0, 0, nextMip.size.x, nextMip.size.y);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           nextMip.texture, 0);

    // Render screen-filled quad of resolution of current mip
    renderer_.DrawMesh(screen_quad_);
  }

  // Disable additive blending
  // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
  glDisable(GL_BLEND);
}

void FinalScene::ApplyHdrPass() noexcept {
  const auto window_size = Engine::window_size();
  glViewport(0, 0, window_size.x, window_size.y);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClear(GL_COLOR_BUFFER_BIT);

  bloom_hdr_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  hdr_fbo_.BindColorBuffer(0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, bloom_fbo_.mip_chain()[0].texture);

  bloom_hdr_pipeline_.SetFloat("bloomStrength", kBloomStrength_);

  renderer_.DrawMesh(screen_quad_);
}

void FinalScene::DrawObjectGeometry(GeometryPipelineType geometry_type) noexcept {
  Pipeline* current_pipeline = nullptr;

  bool is_geometry_pipeline = false;

  switch (geometry_type) {
    case GeometryPipelineType::kGeometry:
      current_pipeline = &geometry_pipeline_;
      is_geometry_pipeline = true;
      break;
    case GeometryPipelineType::kShadowMapping:
      current_pipeline = &shadow_mapping_pipe_;
      is_geometry_pipeline = false;
      break;
    case GeometryPipelineType::kPointShadowMapping:
      current_pipeline = &point_shadow_mapping_pipe_;
      is_geometry_pipeline = false;
      break;
    default:
      return;
  }

  const auto cull_face = is_geometry_pipeline ? GL_BACK : GL_FRONT;
  glEnable(GL_CULL_FACE);
  glCullFace(cull_face);

  // Draw sandstone platform.
  // ------------------------
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0, -11.75f, 0));
  model_ = glm::scale(model_, glm::vec3(0.175f, 0.10, 0.175));
  
  if (is_geometry_pipeline) {
    for (const auto& mesh : sandstone_platform_.meshes()) {
      if (mesh.bounding_sphere().IsOnFrustum(camera_frustum_, model_)) {
        current_pipeline->SetMatrix4("transform.model", model_);
        current_pipeline->SetMatrix4("viewNormalMatrix",
            glm::mat4(glm::transpose(glm::inverse(view_ * model_))));
        
        sandstone_platform_mat_.Bind(GL_TEXTURE0);
        renderer_.DrawModel(sandstone_platform_);
      }
    }
  }
  else {
    current_pipeline->SetMatrix4("transform.model", model_);
    current_pipeline->SetMatrix4("viewNormalMatrix",
        glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

    renderer_.DrawModel(sandstone_platform_);
  }

  // Switch to arm pipeline if we are in geometry pass.
  // --------------------------------------------------
  if (is_geometry_pipeline) {
    arm_geometry_pipe_.Bind();
    arm_geometry_pipe_.SetMatrix4("transform.view", view_);
    arm_geometry_pipe_.SetMatrix4("transform.projection",
                                             projection_);
    current_pipeline = &arm_geometry_pipe_;
  }

  // Draw treasure chest.
  // --------------------
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, treasure_chest_pos_);
  model_ = glm::scale(model_, glm::vec3(4.25f));
  model_ = glm::rotate(model_, glm::radians(22.5f), glm::vec3(0, 1, 0));

  if (is_geometry_pipeline) {
    for (const auto& mesh : treasure_chest_.meshes()) {
      if (mesh.bounding_sphere().IsOnFrustum(camera_frustum_, model_)) {
        current_pipeline->SetMatrix4("transform.model", model_);
        current_pipeline->SetMatrix4("viewNormalMatrix",
            glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

        renderer_.DrawModelWithMaterials(treasure_chest_,
                                         treasure_chest_textures_, 0);
      }
    }
  }
  else {
    current_pipeline->SetMatrix4("transform.model", model_);
    current_pipeline->SetMatrix4("viewNormalMatrix",
        glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

    renderer_.DrawModel(treasure_chest_);
  }

  // Switch to emissive arm pipeline if we are in geometry pass.
  // ------------------------------------------------------------
  if (is_geometry_pipeline) {
    emissive_arm_geometry_pipe_.Bind();
    emissive_arm_geometry_pipe_.SetMatrix4("transform.view", view_);
    emissive_arm_geometry_pipe_.SetMatrix4("transform.projection", projection_);
    current_pipeline = &emissive_arm_geometry_pipe_;
  }

  // Render Leo Magnus.
  // ------------------
  glDisable(GL_CULL_FACE); // The model doesn't handle well the face culling.
  model_ = glm::mat4(1.f);
  const auto leo_pos = glm::vec3(4.f, -2.5f, 3.25f);
  model_ = glm::translate(model_, leo_pos);
  model_ = glm::scale(model_, glm::vec3(40.f));

  if (is_geometry_pipeline) {
    for (const auto& mesh : leo_magnus_.meshes()) {
      if (mesh.bounding_sphere().IsOnFrustum(camera_frustum_, model_)) {
        current_pipeline->SetMatrix4("transform.model", model_);
        current_pipeline->SetMatrix4("viewNormalMatrix",
            glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

        renderer_.DrawModelWithMaterials(leo_magnus_, leo_magnus_textures_, 4);
      }
    }
  } 
  else {
    current_pipeline->SetMatrix4("transform.model", model_);
    current_pipeline->SetMatrix4("viewNormalMatrix",
        glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

    renderer_.DrawModel(leo_magnus_);
  }

  // Render Leo Magnus'sword.
  // ------------------------
  glEnable(GL_CULL_FACE);
  glCullFace(cull_face);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, leo_pos + glm::vec3(0.875f, -0.2f, 1.725f));
  model_ = glm::scale(model_, glm::vec3(40.f));
  model_ = glm::rotate(model_, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));

  if (is_geometry_pipeline) {
    for (const auto& mesh : sword_.meshes()) {
      if (mesh.bounding_sphere().IsOnFrustum(camera_frustum_, model_)) {
        current_pipeline->SetMatrix4("transform.model", model_);
        current_pipeline->SetMatrix4(
            "viewNormalMatrix",
            glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

        renderer_.DrawModelWithMaterials(sword_, sword_textures_, 0);
      }
    }
  } 
  else {
    current_pipeline->SetMatrix4("transform.model", model_);
    current_pipeline->SetMatrix4("viewNormalMatrix",
        glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

    renderer_.DrawModel(sword_);
  }

  glCullFace(GL_BACK);

  current_pipeline = nullptr;
}

void FinalScene::DrawInstancedObjectGeometry(GeometryPipelineType geometry_type)
    noexcept {
  const auto cull_face = geometry_type == GeometryPipelineType::kGeometry ? GL_BACK : GL_FRONT;
  glCullFace(cull_face);

  gold_mat_.Bind(GL_TEXTURE0);

  const auto is_deferred_pipeline =
      geometry_type == GeometryPipelineType::kGeometry;

  if (is_deferred_pipeline) {
    visible_sphere_model_matrices_.clear();

    for (const auto& sphere_model : sphere_model_matrices_) {
      if (sphere_.bounding_sphere().IsOnFrustum(camera_frustum_,
                                                sphere_model)) 
      {
         visible_sphere_model_matrices_.push_back(sphere_model);
      }
    }
  }

  const auto& buffer_data = is_deferred_pipeline 
                               ? visible_sphere_model_matrices_
                               : sphere_model_matrices_;

  sphere_.SetModelMatrixBufferSubData(buffer_data.data(), buffer_data.size());

  const auto sphere_count = is_deferred_pipeline
                                ? visible_sphere_model_matrices_.size()
                                : kSphereCount_;

  // Render spheres.
  renderer_.DrawInstancedMesh(sphere_, sphere_count, GL_TRIANGLE_STRIP);

  glCullFace(GL_BACK);
}

void FinalScene::DestroyPipelines() noexcept {
  equirect_to_cubemap_pipe_.End();
  irradiance_pipeline_.End();
  prefilter_pipeline_.End();
  brdf_pipeline_.End();

  geometry_pipeline_.End();
  instanced_geometry_pipeline_.End();
  arm_geometry_pipe_.End();
  emissive_arm_geometry_pipe_.End();
  ssao_pipeline_.End();
  ssao_blur_pipeline_.End();
  shadow_mapping_pipe_.End();
  point_shadow_mapping_pipe_.End();
  instanced_shadow_mapping_pipe_.End();
  point_instanced_shadow_mapping_pipe_.End();

  pbr_lighting_pipeline_.End();
  debug_lights_pipeline_.End();
  cubemap_pipeline_.End();

  down_sample_pipeline_.End();
  up_sample_pipeline_.End();

  bloom_hdr_pipeline_.End();
}

void FinalScene::DestroyIblPreComputedCubeMaps() noexcept {
  glDeleteTextures(1, &equirectangular_map_);
  glDeleteTextures(1, &env_cubemap_);
  glDeleteTextures(1, &irradiance_cubemap_);
  glDeleteTextures(1, &prefilter_cubemap_);
  glDeleteTextures(1, &brdf_lut_);
}

void FinalScene::DestroyFrameBuffers() noexcept {
  capture_fbo_.Destroy();
  g_buffer_.Destroy();
  ssao_fbo_.Destroy();
  ssao_blur_fbo_.Destroy();
  bloom_fbo_.Destroy();
  hdr_fbo_.Destroy();
}

void FinalScene::DestroyMeshes() noexcept {
  sphere_.Destroy();
  cubemap_mesh_.Destroy();
  screen_quad_.Destroy();
}

void FinalScene::DestroyModels() noexcept { 
  leo_magnus_.Destroy();
  sword_.Destroy();
  sandstone_platform_.Destroy();
  treasure_chest_.Destroy();
}

void FinalScene::DestroyMaterials() noexcept { 
  gold_mat_.Destroy();
  sandstone_platform_mat_.Destroy();

  for (auto& tex : leo_magnus_textures_) {
    glDeleteTextures(1, &tex);
  }

  for (auto& tex : sword_textures_) {
    glDeleteTextures(1, &tex);
  }

  for (auto& tex : treasure_chest_textures_) {
    glDeleteTextures(1, &tex);
  }
}

LoadTextureToGpuJob::LoadTextureToGpuJob(ImageBuffer* image_buffer,
                                         GLuint* texture_id,
                                         const TextureParameters& tex_param) noexcept
  : image_buffer_(image_buffer),
    texture_id_(texture_id),
    texture_param_(tex_param)
{
}

void LoadTextureToGpuJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE
  LoadTextureToGpu(image_buffer_, texture_id_, texture_param_);
}

LoadFileFromDiskJob::LoadFileFromDiskJob(std::string file_path,
                                         FileBuffer* file_buffer) noexcept
    : file_path_(std::move(file_path)),
      file_buffer_(file_buffer)
{
}

void LoadFileFromDiskJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
  ZoneText(file_path_.data(), file_path_.size());
#endif  // TRACY_ENABLE

  file_utility::LoadFileInBuffer(file_path_.data(), file_buffer_);
}

PipelineCreationJob::PipelineCreationJob(
  FileBuffer* v_shader_buff,
  FileBuffer* f_shader_buff, Pipeline* pipeline)
  : vertex_shader_buffer_(v_shader_buff),
    fragment_shader_buffer_(f_shader_buff),
    pipeline_(pipeline) {}

void PipelineCreationJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  pipeline_->Begin(*vertex_shader_buffer_, 
      *fragment_shader_buffer_);
}

FunctionExecutionJob::FunctionExecutionJob(
    const std::function<void()>& func) noexcept
    :
  function_(func)
{}

void FunctionExecutionJob::Work() noexcept {
  // Tracy already in the scope of the function.
  function_();
}

ModelCreationJob::ModelCreationJob(Model* model, const std::string_view file_path,
                               const bool gamma, const bool flip_y) noexcept
    : model_(model),
      file_path_(file_path),
      gamma_(gamma),
      flip_y_(flip_y)
{
}

void ModelCreationJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  model_->Load(file_path_, gamma_, flip_y_);
  model_->GenerateModelSphereBoundingVolume();
}

LoadModelToGpuJob::LoadModelToGpuJob(Model* model) noexcept :
  model_(model) {}

void LoadModelToGpuJob::Work() noexcept {
#ifdef TRACY_ENABLE
  ZoneScoped;
#endif  // TRACY_ENABLE

  model_->LoadToGpu();
}
