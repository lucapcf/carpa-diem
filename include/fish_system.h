#ifndef FISH_SYSTEM_H
#define FISH_SYSTEM_H

#include <glm/vec3.hpp>

// =====================================================================
// Tipos de peixes (baseados nos modelos disponíveis)
// =====================================================================
enum FishType {
    FISH_ANGLER,        // Peixe-pescador - raro, difícil
    FISH_BLOWFISH,      // Baiacu - comum, fácil
    FISH_KINGFISH,      // Peixe-rei - raro, difícil
    FISH_TROUT,         // Truta - comum, moderado
    FISH_PIRANHA,       // Piranha - moderado, agressivo
    FISH_TYPE_COUNT
};

// Informações de cada tipo de peixe
struct FishTypeInfo {
    const char* name;           // Nome para exibição
    const char* model_path;     // Caminho do modelo .obj
    const char* shape_name;     // Nome da shape para DrawVirtualObject
    const char* texture_path;   // Caminho da textura (nullptr = usa textura padrão)
    int texture_id;             // ID da textura carregada (-1 = usa padrão)
    float spawn_chance;         // Chance de spawn (soma = 1.0)
    float min_speed;            // Velocidade mínima
    float max_speed;            // Velocidade máxima
    float catch_chance;         // Chance de fisgar (0.0 a 1.0)
    int points;                 // Pontos ao capturar
    float scale;                // Escala do modelo
};

// Tabela de tipos (definida em fish_system.cpp)
extern FishTypeInfo g_FishTypes[FISH_TYPE_COUNT];

// =====================================================================
// Sistema de movimento (Catmull-Rom)
// =====================================================================
#define NUM_WAYPOINTS 4

struct RandomFish {
    FishType type;                    // Tipo do peixe
    glm::vec3 position;               // Posição atual
    glm::vec3 waypoints[NUM_WAYPOINTS];
    float segment_t;
    float speed;
    float rotation_y;
    bool is_hooked;                   // Se foi fisgado
    
    RandomFish() : type(FISH_TROUT), position(0.0f), segment_t(0.0f), 
                   speed(0.5f), rotation_y(0.0f), is_hooked(false) {
        for (int i = 0; i < NUM_WAYPOINTS; i++) waypoints[i] = glm::vec3(0.0f);
    }
};

extern RandomFish g_RandomFish;

// =====================================================================
// Funções
// =====================================================================
void InitializeFishSystem();
void UpdateFish(float delta_time);
void UpdateRandomFish(float delta_time);
void GenerateFishRoute(int area);
void SpawnRandomFish(int area);
void ResetFish();

// Retorna info do peixe atual
const FishTypeInfo& GetCurrentFishInfo();

// Funções de acesso ao peixe aleatório (para uso em collisions.cpp)
glm::vec3 GetRandomFishPosition();
const FishTypeInfo& GetRandomFishInfo();
bool IsRandomFishHooked();
void SetRandomFishHooked(bool hooked);

// Tenta fisgar o peixe (retorna true se conseguiu)
bool TryHookFish(const glm::vec3& hook_pos, float radius);

#endif // FISH_SYSTEM_H
