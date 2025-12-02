#include "collision.h"
#include <glm/geometric.hpp>

// Função para testar colisão AABB-AABB
bool TestAABBAABB(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Função para testar colisão Esfera-Esfera
bool TestSphereSphere(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2) {
    float distance = glm::length(center1 - center2);
    return distance <= (radius1 + radius2);
}

// Função para testar colisão Esfera-Plano
bool TestSpherePlane(glm::vec3 sphere_center, float sphere_radius, float plane_y) {
    return (sphere_center.y - sphere_radius) <= plane_y;
}