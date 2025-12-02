#include "collisions.h"
#include "fish_system.h"
#include "game_types.h"

#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <cstdio>

// =====================================================================
// Funções de teste de colisão básicas
// =====================================================================

bool TestSphereSphere(const glm::vec3& center1, float radius1, const glm::vec3& center2, float radius2) {
    float distance = glm::length(center1 - center2);
    return distance <= (radius1 + radius2);
}

bool TestSpherePlane(const glm::vec3& sphere_center, float sphere_radius, float plane_y) {
    return (sphere_center.y - sphere_radius) <= plane_y;
}

bool TestAABBAABB(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// =====================================================================
// Funções de colisão específicas do jogo
// =====================================================================

bool CheckBaitFishCollision(const Bait& bait, float fish_radius) {
    // Só verifica colisão se a isca está na água
    if (!bait.is_in_water) return false;
    
    // Obtém a posição do peixe aleatório atual
    glm::vec3 fish_pos = GetRandomFishPosition();
    
    // Colisão esfera-esfera entre isca e peixe
    return TestSphereSphere(bait.position, bait.radius, fish_pos, fish_radius);
}

HookResult TryHookRandomFish(const Bait& bait, float fish_radius) {
    // Só verifica se a isca está na água
    if (!bait.is_in_water) return HOOK_NO_COLLISION;
    
    // Se o peixe já foi fisgado, retorna sucesso
    if (IsRandomFishHooked()) return HOOK_FISH_CAUGHT;
    
    // Obtém a posição do peixe aleatório atual
    glm::vec3 fish_pos = GetRandomFishPosition();
    
    // Verifica colisão esfera-esfera
    if (!TestSphereSphere(bait.position, bait.radius, fish_pos, fish_radius)) {
        return HOOK_NO_COLLISION;
    }
    
    // Houve colisão! Agora usa a probabilidade do tipo de peixe
    const FishTypeInfo& info = GetRandomFishInfo();
    
    // Sorteia número entre 0 e 1
    float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    
    // Se o sorteio for menor ou igual à catch_chance, peixe foi fisgado
    if (roll <= info.catch_chance) {
        SetRandomFishHooked(true);
        printf("[Fish] %s fisgado! (+%d pontos)\n", info.name, info.points);
        return HOOK_FISH_CAUGHT;
    }
    
    // Senão, o peixe escapou
    printf("[Fish] %s escapou! (%.0f%% chance, roll=%.2f)\n", 
           info.name, info.catch_chance * 100.0f, roll);
    return HOOK_FISH_ESCAPED;
}

int GetHookedFishPoints() {
    if (!IsRandomFishHooked()) return 0;
    return GetRandomFishInfo().points;
}

const char* GetCurrentFishName() {
    return GetRandomFishInfo().name;
}
