#include "shapes.h"

#include <algorithm>

float Plane::CalculatePointDistanceToPlane(const glm::vec3& point) const {
  return glm::dot(normal_, point) - distance_;
}

bool BoundingSphere::IsOnFrustum(const Frustum& cam_frustum,
                                 const glm::mat4& model) const {
  // Global scale is computed by doing the magnitude of
  // X, Y and Z model matrix's column.
  const glm::vec3 global_scale(glm::length(model[0]), glm::length(model[1]),
                               glm::length(model[2]));
  // Get our global center with process it with the global model matrix of our
  // transform.
  const glm::vec3 global_center(model * glm::vec4(center_, 1.f));

  // To wrap correctly our shape, we need the maximum scale scalar.
  const float maxScale = std::max(std::max(global_scale.x, global_scale.y), 
                                           global_scale.z);

  // Max scale is assuming for the diameter. So, we need the half to apply it to
  //our radius.
  BoundingSphere global_sphere(global_center, radius_ * maxScale);

  // Check Firstly the result that have the most chance
  // to faillure to avoid to call all functions.
  return (global_sphere.IsOnOrForwardPlane(cam_frustum.left_face) &&
          global_sphere.IsOnOrForwardPlane(cam_frustum.right_face)  &&
          global_sphere.IsOnOrForwardPlane(cam_frustum.far_face)  &&
          global_sphere.IsOnOrForwardPlane(cam_frustum.near_face) &&
          global_sphere.IsOnOrForwardPlane(cam_frustum.top_face) &&
          global_sphere.IsOnOrForwardPlane(cam_frustum.bottom_face));
}

bool BoundingSphere::IsOnOrForwardPlane(const Plane& plane) const {
  const auto distance = plane.CalculatePointDistanceToPlane(center_);
  return distance > -(radius_);
}