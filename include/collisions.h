#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/vec3.hpp>
#include "game_types.h"

// =====================================================================
// Funções de teste de colisão
// =====================================================================

// Teste de colisão entre duas esferas
bool TestSphereSphere(const glm::vec3& center1, float radius1, const glm::vec3& center2, float radius2);

// Teste de colisão entre esfera e plano horizontal (Y constante)
bool TestSpherePlane(const glm::vec3& sphere_center, float sphere_radius, float plane_y);

// Teste de colisão entre duas AABBs
bool TestAABBAABB(const AABB& a, const AABB& b);

// =====================================================================
// Funções de colisão específicas do jogo
// =====================================================================

// Resultado da tentativa de fisgar um peixe
enum HookResult {
    HOOK_NO_COLLISION,  // Isca não está perto do peixe
    HOOK_FISH_ESCAPED,  // Colidiu mas o peixe escapou
    HOOK_FISH_CAUGHT    // Peixe foi fisgado com sucesso!
};

// Verifica colisão entre a isca e o peixe aleatório atual
// Retorna true se houve colisão (independente de ter fisgado ou não)
bool CheckBaitFishCollision(const Bait& bait, float fish_radius = 0.3f);

// Tenta fisgar o peixe aleatório quando a isca está próxima
// Usa probabilidade baseada no tipo do peixe
// Retorna o resultado da tentativa (sem colisão, escapou, ou fisgado)
HookResult TryHookRandomFish(const Bait& bait, float fish_radius = 0.3f);

// Retorna os pontos ganhos se o peixe atual foi fisgado
// Retorna 0 se não foi fisgado
int GetHookedFishPoints();

// Retorna o nome do peixe atual (para exibição)
const char* GetCurrentFishName();

#endif // COLLISIONS_H
