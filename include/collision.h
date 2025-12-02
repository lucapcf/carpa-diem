#ifndef COLLISION_H
#define COLLISION_H

#include <glm/vec3.hpp>

// Estrutura para Axis-Aligned Bounding Box
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Funções de detecção de colisão
bool TestAABBAABB(const AABB& a, const AABB& b);
bool TestSphereSphere(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2);
bool TestSpherePlane(glm::vec3 sphere_center, float sphere_radius, float plane_y);

#endif // COLLISION_H