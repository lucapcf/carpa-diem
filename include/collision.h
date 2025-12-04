#ifndef COLLISION_H
#define COLLISION_H

#include <glm/vec3.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(glm::vec3 min_val, glm::vec3 max_val) : min(min_val), max(max_val) {}
};

bool TestAABBAABB(const AABB& a, const AABB& b);
bool TestAABBSphere(const AABB& box, glm::vec3 sphere_center, float sphere_radius);
bool TestSphereSphere(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2);
bool TestSpherePlane(glm::vec3 sphere_center, float sphere_radius, float plane_y);

#endif // COLLISION_H