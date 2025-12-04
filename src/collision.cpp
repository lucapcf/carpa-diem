#include "collision.h"
#include <glm/geometric.hpp>
#include <algorithm>

bool TestAABBAABB(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool TestAABBSphere(const AABB& box, glm::vec3 sphere_center, float sphere_radius) {
    float closest_x = std::max(box.min.x, std::min(sphere_center.x, box.max.x));
    float closest_y = std::max(box.min.y, std::min(sphere_center.y, box.max.y));
    float closest_z = std::max(box.min.z, std::min(sphere_center.z, box.max.z));

    glm::vec3 closest_point(closest_x, closest_y, closest_z);

    float distance = glm::length(sphere_center - closest_point);
    return distance <= sphere_radius;
}

bool TestSphereSphere(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2) {
    float distance = glm::length(center1 - center2);
    return distance <= (radius1 + radius2);
}

bool TestSpherePlane(glm::vec3 sphere_center, float sphere_radius, float plane_y) {
    return (sphere_center.y - sphere_radius) <= plane_y;
}
