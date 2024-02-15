#include "camera.h"

#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <array>

void Camera::Begin(const glm::vec3 pos, float fov, float near, float far, 
                   float yaw, float pitch, const glm::vec3 front) noexcept {
  position_ = pos;
  fov_ = fov;
  near_ = near;
  far_ = far;
  yaw_ = yaw;
  pitch_ = pitch;
  front_ = front;
  UpdateCameraVectors(); 
}

void Camera::End() noexcept { 
  position_ = glm::vec3(0.f, 0.f, 0.f);
  front_ = glm::vec3(0.f, 0.f, -1.f);
  yaw_ = -90.f, pitch_ = 0.f;
  fov_ = 45.f;
  near_ = 0.1f;
  far_ = 100.f;
  
  UpdateCameraVectors();
  SDL_SetRelativeMouseMode(SDL_FALSE);
}

void Camera::Update(const float dt) { 
  if (are_mouse_inputs_enabled_) {
    Zoom(wheel_scroll_, dt);
  }
  Rotate(dt);
  Move(dt);
  wheel_scroll_ = 0.f;
}

void Camera::OnEvent(const SDL_Event& event) noexcept {
  static SDL_bool relative_mode = SDL_FALSE;

  switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_ESCAPE:
          relative_mode = relative_mode == SDL_TRUE ? SDL_FALSE : SDL_TRUE;
          SDL_SetRelativeMouseMode(relative_mode);
          break;
        default:
          break;
      }
      break;
    case SDL_MOUSEWHEEL:
      wheel_scroll_ = event.wheel.y;
      break;
    case SDL_MOUSEBUTTONDOWN:
      switch (event.button.button) {
        case SDL_BUTTON_RIGHT:
          relative_mode = relative_mode == SDL_TRUE ? SDL_FALSE : SDL_TRUE;
          SDL_SetRelativeMouseMode(relative_mode);
          break;
        default:
          break;
      }
      break;

    default:
  	  break;
  }
}

void Camera::Move(const float dt) noexcept {
  // Check keys that are currently pressed
  const Uint8* keys = SDL_GetKeyboardState(nullptr);

  current_move_speed_ = keys[SDL_SCANCODE_LCTRL] ? max_movement_speed_ : 
                                                   min_movement_speed_;

  if (keys[SDL_SCANCODE_W]) position_ += front_ * (current_move_speed_ * dt);
  if (keys[SDL_SCANCODE_S]) position_ -= front_ * (current_move_speed_ * dt);
  if (keys[SDL_SCANCODE_A]) position_ -= right_ * (current_move_speed_ * dt);
  if (keys[SDL_SCANCODE_D]) position_ += right_ * (current_move_speed_ * dt);
  if (keys[SDL_SCANCODE_SPACE]) position_ += world_up_ * (current_move_speed_ * dt);
  if (keys[SDL_SCANCODE_LSHIFT]) position_ -= world_up_ * (current_move_speed_ * dt);
}

void Camera::Rotate(const float dt) noexcept {
  if (!SDL_GetRelativeMouseMode()) {
    return;
  }

  int mouse_x, mouse_y;
  SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

  yaw_ += mouse_x * sensitivity_ * dt;
  pitch_ -= mouse_y * sensitivity_ * dt;

  if (pitch_ > 89.0f) pitch_ = 89.0f;
  if (pitch_ < -89.0f) pitch_ = -89.0f;

  UpdateCameraVectors();
}

void Camera::Zoom(float wheel_scroll, float dt) noexcept {
  fov_ -= wheel_scroll * wheel_scroll_speed_ * dt;  // * dt
  if (fov_ < 1.0f) fov_ = 1.0f;
  if (fov_ > 45.0f) fov_ = 45.0f;
}

void Camera::UpdateCameraVectors() noexcept {
  glm::vec3 new_front;
  new_front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  new_front.y = sin(glm::radians(pitch_));
  new_front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front_ = glm::normalize(new_front);

  // normalize the vectors, because their length gets closer to 0 the more you
  // look up or down which results in slower movement
  right_ = glm::normalize(glm::cross(front_, world_up_));
  up_ = glm::normalize(glm::cross(right_, front_));
}

const glm::mat4 Camera::CalculateViewMatrix() const noexcept {
  return glm::lookAt(position_, position_ + front_, up_);
}

const glm::mat4 Camera::CalculateProjectionMatrix(float aspect_ratio) const noexcept {
  return glm::perspective(glm::radians(fov_), aspect_ratio, near_, far_);
}

const Frustum Camera::CalculateFrustum(float aspect_ratio) noexcept {
  // Half vertical side.
  const float far_half_v_side = far_ * std::tan(glm::radians(fov_ * 0.5f));
  // Half horizontal side.
  const float far_half_h_side = far_half_v_side * aspect_ratio;

  const glm::vec3 front_mul_far = far_ * front_;

  Frustum frustum;

  // Near and Far plane.
  frustum.near_face = Plane(position_ + near_ * front_, front_);
  frustum.far_face = Plane(position_ + front_mul_far, -front_);

  // Right and Left plane.
  frustum.left_face = Plane(
      position_, glm::cross(front_mul_far - right_ * far_half_h_side, up_));
  frustum.right_face = Plane(
      position_, glm::cross(up_, front_mul_far + right_ * far_half_h_side));

  // Top and Bottom plane.
  frustum.bottom_face = Plane(
      position_, glm::cross(right_, front_mul_far - up_ * far_half_v_side));

  frustum.top_face = Plane(
      position_, glm::cross(front_mul_far + up_ * far_half_v_side, right_));

  // Calculate frustum corners
  float near_half_h = near_ * std::tan(glm::radians(fov_ * 0.5f));
  float near_half_w = near_half_h * aspect_ratio;

  float far_half_h = far_ * std::tan(glm::radians(fov_ * 0.5f));
  float far_half_w = far_half_h * aspect_ratio;

  glm::vec3 near_center = position_ + near_ * front_;
  glm::vec3 far_center = position_ + far_ * front_;

  // Near plane corners
  frustum.corners[0] = near_center + up_ * near_half_h - right_ * near_half_w;
  frustum.corners[1] = near_center - up_ * near_half_h - right_ * near_half_w;
  frustum.corners[2] = near_center + up_ * near_half_h + right_ * near_half_w;
  frustum.corners[3] = near_center - up_ * near_half_h + right_ * near_half_w;

  // Far plane corners
  frustum.corners[4] = far_center + up_ * far_half_h - right_ * far_half_w;
  frustum.corners[5] = far_center - up_ * far_half_h - right_ * far_half_w;
  frustum.corners[6] = far_center + up_ * far_half_h + right_ * far_half_w;
  frustum.corners[7] = far_center - up_ * far_half_h + right_ * far_half_w;

  return frustum;
}