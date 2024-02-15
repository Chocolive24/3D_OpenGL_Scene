#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <array>

class Plane {
public:
  Plane() = default;

  // Create a plane with a point and a normal.
  Plane(glm::vec3 point, glm::vec3 norm)
    : normal_(glm::normalize(norm)), distance_(glm::dot(normal_, point)) {}

  float CalculatePointDistanceToPlane(const glm::vec3& point) const;

  glm::vec3 normal_ = glm::vec3(0.f, 1.f, 0.f);

  // Distance from origin to the nearest point in the plane.
  float distance_ = 0.f;
};

struct Frustum {
  Plane top_face;
  Plane bottom_face;

  Plane right_face;
  Plane left_face;

  Plane far_face;
  Plane near_face;

  std::array<glm::vec3, 8> corners;
};

class BoundingVolume {
public:
  [[nodiscard]] virtual bool IsOnFrustum(const Frustum& cam_frustum, 
                                         const glm::mat4& model) const = 0;

private:
  [[nodiscard]] virtual bool IsOnOrForwardPlane(const Plane& plane) const = 0;
};

class BoundingSphere final : public BoundingVolume {
public:
  BoundingSphere() = default;

  BoundingSphere(glm::vec3 in_center, float in_radius)
      : BoundingVolume(), center_(in_center), radius_(in_radius) 
  {}

  [[nodiscard]] bool IsOnFrustum(const Frustum& cam_frustum,
                                 const glm::mat4& modelViewProjection) const override;

private:
  [[nodiscard]] bool IsOnOrForwardPlane(const Plane& plane) const override;

  glm::vec3 center_{0.f, 0.f, 0.f};
  float radius_ = 0.f;
};