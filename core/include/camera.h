#pragma once

#include "shapes.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <SDL_events.h>

class Camera {
public:
  void Begin(glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f),
			 float fov = 45.f, 
			 float near = 0.1f, 
			 float far = 100.f,
			 float yaw = -90.f,
			 float pitch = 0.f,
			 glm::vec3 front = glm::vec3(0.f, 0.f, -1.f)) noexcept;
  void Update(float dt);
  void OnEvent(const SDL_Event& event) noexcept;
  void End() noexcept;

  [[nodiscard]] const glm::mat4 CalculateViewMatrix() const noexcept;
  [[nodiscard]] const glm::mat4 CalculateProjectionMatrix(
      float aspect_ratio) const noexcept;

  [[nodiscard]] const Frustum CalculateFrustum(float aspect_ratio) noexcept;

  [[nodiscard]] const float fov() const noexcept { return fov_; }
  [[nodiscard]] const float near() const noexcept { return near_; }
  [[nodiscard]] const float far() const noexcept { return far_; }

  [[nodiscard]] const glm::vec3 position() const noexcept { return position_; }
  [[nodiscard]] const glm::vec3 front() const noexcept { return front_; }

  [[nodiscard]] const float yaw() const noexcept { return yaw_; }
  [[nodiscard]] const float pitch() const noexcept { return pitch_; }

  void ChangeMouseInputsEnability(bool enable) noexcept {
    are_mouse_inputs_enabled_ = enable;
  }

private:
  static constexpr glm::vec3 world_up_ = glm::vec3(0.f, 1.f, 0.f);

  glm::vec3 position_;
  glm::vec3 front_;
  glm::vec3 right_;
  glm::vec3 up_;

  float yaw_ = -90.f, pitch_ = 0.f;
  float fov_ = 45.f, near_ = 0.1f, far_ = 100.f;

  float min_movement_speed_ = 3.f, max_movement_speed_ = 6.f;
  float current_move_speed_ = 0.f;
  float sensitivity_ = 15.f;
  float wheel_scroll_ = 0.f, wheel_scroll_speed_ = 200.f;

  bool are_mouse_inputs_enabled_ = true;

  void Move(float dt) noexcept;
  void Rotate(float dt) noexcept;
  void Zoom(float wheel_scroll, float dt) noexcept;
  void UpdateCameraVectors() noexcept;
};