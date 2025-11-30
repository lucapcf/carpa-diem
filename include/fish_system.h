#ifndef FISH_SYSTEM_H
#define FISH_SYSTEM_H

#include <glm/vec3.hpp>

// =====================================================================
// Sistema para gerenciar peixes com movimento aleatório suave (Bézier)
// =====================================================================

// Número de waypoints na fila (posição atual + pontos futuros)
#define NUM_WAYPOINTS 4

// Estrutura para o peixe com movimento aleatório usando curva de Bézier contínua
struct RandomFish {
    glm::vec3 position;               // Posição atual do peixe
    glm::vec3 waypoints[NUM_WAYPOINTS]; // Fila de waypoints (0=atual, 1,2,3=futuros)
    float segment_t;                  // Parâmetro t do segmento atual (0 a 1)
    float speed;                      // Velocidade de percurso
    float rotation_y;                 // Rotação atual
    
    RandomFish() : position(0.0f), segment_t(0.0f), speed(0.5f), rotation_y(0.0f) {
        for (int i = 0; i < NUM_WAYPOINTS; i++) waypoints[i] = glm::vec3(0.0f);
    }
};

// Peixe gerenciado pelo fish_system (exportado para uso em main.cpp)
extern RandomFish g_RandomFish;

// Inicializa o sistema de peixes
void InitializeFishSystem();

// Atualiza a posição do peixe antigo (usando Bézier)
void UpdateFish(float delta_time);

// Atualiza o peixe com movimento aleatório suave
void UpdateRandomFish(float delta_time);

// Gera uma nova rota para o peixe antigo em uma área específica
void GenerateFishRoute(int area);

// Gera o peixe aleatório em uma área específica
void SpawnRandomFish(int area);

// Reseta o peixe
void ResetFish();

// Retorna o nome do objeto 3D do peixe para renderização
const char* GetFishModelName();

#endif // FISH_SYSTEM_H
