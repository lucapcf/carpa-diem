#include "fish_system.h"
#include "game_state.h"
#include "game_types.h"

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>

// =====================================================================
// Tabela de tipos de peixes
// =====================================================================
// { name, model_path, shape_name, texture_path, texture_id, spawn_chance, min_speed, max_speed, catch_chance, points, scale }
// Nota: Angler Fish e Blowfish usam cores de material (sem textura de imagem)
FishTypeInfo g_FishTypes[FISH_TYPE_COUNT] = {
    { "Peixe-Pescador", "../../data/models/fishs/Angler Fish/model.obj",      "AnglerFish",         nullptr, -1, 0.20f, 0.3f, 0.5f, 0.40f, 100, 0.5f },
    { "Baiacu",         "../../data/models/fishs/Blowfish/Blowfish_01.obj",   "Blowfish_01_Sphere", nullptr, -1, 0.25f, 0.2f, 0.4f, 0.80f,  15, 0.05f },
    { "Peixe-Rei",      "../../data/models/fishs/Kingfish/Mesh_Kingfish.obj", "Geo_Kingfish",       "../../data/models/fishs/Kingfish/Tex_Kingfish.png", -1, 0.10f, 0.5f, 0.8f, 0.35f,  80, 0.01f },
    { "Truta",          "../../data/models/fishs/Trout/Mesh_Trout.obj",       "Geo_Trout",          "../../data/models/fishs/Trout/Tex_Trout.png", -1, 0.35f, 0.4f, 0.7f, 0.70f,  25, 0.015f },
    { "Piranha",        "../../data/models/fishs/Piranha/Piranha.obj",       "Piranha_mesh",       "../../data/models/fishs/Piranha/Piranha_BaseColor.png", -1, 0.20f, 0.4f, 0.7f, 0.50f,  60, 0.15f }
};

// =====================================================================
// Configurações da área de natação do peixe
// =====================================================================
static const float SWIM_DEPTH_MIN  = -1.5f;
static const float SWIM_DEPTH_MAX  = -0.3f;
static const float MIN_MOVE_DISTANCE = 1.0f;
static const float MAX_MOVE_DISTANCE = 3.0f;
static const float GRID_MARGIN = 1.0f;
static const float GRID_MIN_X = -10.0f + GRID_MARGIN;
static const float GRID_MAX_X =  10.0f - GRID_MARGIN;
static const float GRID_MIN_Z = -10.0f + GRID_MARGIN;
static const float GRID_MAX_Z =  10.0f - GRID_MARGIN;
static const float ARRIVAL_THRESHOLD = 0.3f;
static const float HOOK_RADIUS = 1.0f;

static bool g_RandomInitialized = false;
static glm::vec3 g_FishDestination;

// =====================================================================
// Peixe com movimento aleatório
// =====================================================================
RandomFish g_RandomFish;

// =====================================================================
// Funções auxiliares
// =====================================================================

static float RandomFloat(float min, float max) {
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    float normalized = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + normalized * (max - min);
}

// Escolhe tipo de peixe baseado nas probabilidades de spawn
static FishType ChooseRandomFishType() {
    float roll = RandomFloat(0.0f, 1.0f);
    float cumulative = 0.0f;
    
    for (int i = 0; i < FISH_TYPE_COUNT; i++) {
        cumulative += g_FishTypes[i].spawn_chance;
        if (roll <= cumulative) {
            return static_cast<FishType>(i);
        }
    }
    return FISH_TROUT;  // Fallback
}

// Gera um novo destino aleatório próximo à posição atual do peixe
static void GenerateNewDestination() {
    // Gera uma direção aleatória (ângulo em radianos)
    float angle = RandomFloat(0.0f, 2.0f * 3.14159f);
    
    // Gera uma distância aleatória para o próximo ponto
    float distance = RandomFloat(MIN_MOVE_DISTANCE, MAX_MOVE_DISTANCE);
    
    // Calcula o novo destino baseado na posição ATUAL do peixe
    g_FishDestination = glm::vec3(
        g_Fish.position.x + cos(angle) * distance,
        RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
        g_Fish.position.z + sin(angle) * distance
    );
}

// =====================================================================
// Funções do sistema de peixes
// =====================================================================

const FishTypeInfo& GetCurrentFishInfo() {
    return g_FishTypes[g_RandomFish.type];
}

void InitializeFishSystem() {
    printf("=================================================\n");
    printf("Inicializando sistema de peixes...\n");
    printf("=================================================\n");
    
    // Inicializa gerador de números aleatórios
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    
    // Mostra tipos disponíveis
    printf("Tipos de peixes:\n");
    for (int i = 0; i < FISH_TYPE_COUNT; i++) {
        printf("  %s - Spawn: %.0f%%, Fisgar: %.0f%%, Pts: %d\n",
               g_FishTypes[i].name, 
               g_FishTypes[i].spawn_chance * 100.0f,
               g_FishTypes[i].catch_chance * 100.0f,
               g_FishTypes[i].points);
    }
    
    // Valores iniciais
    g_Fish.position = glm::vec3(0.0f, -0.5f, 0.0f);
    g_Fish.speed = 0.8f;
    g_Fish.rotation_y = 0.0f;
    g_FishDestination = g_Fish.position;
    
    printf("Sistema de peixes inicializado!\n");
}

void UpdateFish(float delta_time) {
    // Vetor do peixe até o destino
    glm::vec3 to_destination = g_FishDestination - g_Fish.position;
    float distance = glm::length(to_destination);
    
    // Se chegou perto do destino, gera um novo destino aleatório
    if (distance < ARRIVAL_THRESHOLD) {
        GenerateNewDestination();
        to_destination = g_FishDestination - g_Fish.position;
        distance = glm::length(to_destination);
    }
    
    // Direção normalizada para o destino
    glm::vec3 direction = (distance > 0.001f) ? to_destination / distance : glm::vec3(0.0f);
    
    // Move o peixe em direção ao destino
    float move_distance = g_Fish.speed * delta_time;
    if (move_distance > distance) {
        move_distance = distance;  // Não ultrapassa o destino
    }
    
    g_Fish.position += direction * move_distance;
    
    // Atualiza rotação para olhar na direção do movimento
    if (glm::length(direction) > 0.001f) {
        g_Fish.rotation_y = atan2(direction.x, direction.z);
    }
}

void GenerateFishRoute(int area) {
    // Pega o centro da área de pesca para posição inicial
    glm::vec3 area_center = g_MapAreas[area].center;
    
    // Posiciona o peixe próximo ao centro da área (com variação aleatória)
    g_Fish.position = glm::vec3(
        area_center.x + RandomFloat(-1.5f, 1.5f),
        RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
        area_center.z + RandomFloat(-1.5f, 1.5f)
    );
    
    g_Fish.speed = 0.8f;
    g_Fish.rotation_y = 0.0f;
    
    // Gera o primeiro destino a partir da posição atual
    GenerateNewDestination();
    
    printf("Novo peixe gerado na area %d em (%.2f, %.2f, %.2f)\n", 
           area, g_Fish.position.x, g_Fish.position.y, g_Fish.position.z);
    printf("Primeiro destino: (%.2f, %.2f, %.2f)\n",
           g_FishDestination.x, g_FishDestination.y, g_FishDestination.z);
}

void ResetFish() {
    g_Fish.rotation_y = 0.0f;
    GenerateNewDestination();
}

// =====================================================================
// Funções para o peixe com movimento aleatório suave (Catmull-Rom Spline)
// =====================================================================

// Catmull-Rom spline - passa suavemente por todos os pontos de controle
// t varia de 0 a 1 entre p1 e p2
// p0 e p3 são usados para calcular as tangentes
static glm::vec3 CatmullRom(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    // Fórmula Catmull-Rom
    glm::vec3 result = 0.5f * (
        (2.0f * p1) +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
    );
    
    return result;
}

// Limita um ponto às coordenadas do grid
static glm::vec3 ClampToGrid(glm::vec3 point) {
    point.x = glm::clamp(point.x, GRID_MIN_X, GRID_MAX_X);
    point.z = glm::clamp(point.z, GRID_MIN_Z, GRID_MAX_Z);
    return point;
}

// Gera um ponto aleatório próximo a uma posição base, respeitando os limites do grid
static glm::vec3 GenerateRandomPointNear(glm::vec3 base_position) {
    glm::vec3 new_point;
    int attempts = 0;
    const int max_attempts = 10;
    
    do {
        float angle = RandomFloat(0.0f, 2.0f * 3.14159f);
        float distance = RandomFloat(MIN_MOVE_DISTANCE, MAX_MOVE_DISTANCE);
        
        new_point = glm::vec3(
            base_position.x + cos(angle) * distance,
            RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
            base_position.z + sin(angle) * distance
        );
        
        // Se o ponto está fora do grid, tenta direcionar para o centro
        if (new_point.x < GRID_MIN_X || new_point.x > GRID_MAX_X ||
            new_point.z < GRID_MIN_Z || new_point.z > GRID_MAX_Z) {
            
            // Direciona o peixe de volta para o centro do grid
            glm::vec3 to_center = glm::vec3(0.0f, 0.0f, 0.0f) - base_position;
            if (glm::length(to_center) > 0.001f) {
                to_center = glm::normalize(to_center);
                // Adiciona variação para não ir em linha reta
                float center_angle = atan2(to_center.x, to_center.z);
                center_angle += RandomFloat(-0.5f, 0.5f);  // ±30 graus de variação
                
                new_point = glm::vec3(
                    base_position.x + cos(center_angle) * distance,
                    RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
                    base_position.z + sin(center_angle) * distance
                );
            }
        }
        
        attempts++;
    } while (attempts < max_attempts && 
             (new_point.x < GRID_MIN_X || new_point.x > GRID_MAX_X ||
              new_point.z < GRID_MIN_Z || new_point.z > GRID_MAX_Z));
    
    // Garante que o ponto final está dentro do grid
    return ClampToGrid(new_point);
}

// Avança a fila de waypoints: remove o primeiro e adiciona um novo no final
static void AdvanceWaypoints() {
    // Shift: waypoint[0] <- waypoint[1] <- waypoint[2] <- waypoint[3]
    g_RandomFish.waypoints[0] = g_RandomFish.waypoints[1];
    g_RandomFish.waypoints[1] = g_RandomFish.waypoints[2];
    g_RandomFish.waypoints[2] = g_RandomFish.waypoints[3];
    
    // Gera novo waypoint baseado no último
    g_RandomFish.waypoints[3] = GenerateRandomPointNear(g_RandomFish.waypoints[2]);
}

void SpawnRandomFish(int area) {
    // Inicializa random se necessário
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    
    // Escolhe tipo de peixe aleatório
    g_RandomFish.type = ChooseRandomFishType();
    const FishTypeInfo& info = g_FishTypes[g_RandomFish.type];
    
    // Pega o centro da área de pesca para posição inicial
    glm::vec3 area_center = g_MapAreas[area].center;
    
    // Posição inicial do peixe (clampada ao grid)
    glm::vec3 start_pos = ClampToGrid(glm::vec3(
        area_center.x + RandomFloat(-2.0f, 2.0f),
        RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
        area_center.z + RandomFloat(-2.0f, 2.0f)
    ));
    
    // Inicializa waypoints
    g_RandomFish.waypoints[0] = ClampToGrid(start_pos + glm::vec3(RandomFloat(-1.0f, 1.0f), 0.0f, RandomFloat(-1.0f, 1.0f)));
    g_RandomFish.waypoints[1] = start_pos;
    g_RandomFish.waypoints[2] = GenerateRandomPointNear(g_RandomFish.waypoints[1]);
    g_RandomFish.waypoints[3] = GenerateRandomPointNear(g_RandomFish.waypoints[2]);
    
    g_RandomFish.position = start_pos;
    g_RandomFish.segment_t = 0.0f;
    g_RandomFish.speed = RandomFloat(info.min_speed, info.max_speed);
    g_RandomFish.rotation_y = RandomFloat(0.0f, 2.0f * 3.14159f);
    g_RandomFish.is_hooked = false;
    
    printf("[Fish] %s apareceu! (Vel: %.2f, Fisgar: %.0f%%, Pts: %d)\n", 
           info.name, g_RandomFish.speed, info.catch_chance * 100.0f, info.points);
}

void UpdateRandomFish(float delta_time) {
    // Guardar posição anterior para calcular direção
    glm::vec3 old_position = g_RandomFish.position;
    
    // Avançar no segmento atual (de waypoint[1] para waypoint[2])
    g_RandomFish.segment_t += g_RandomFish.speed * delta_time;
    
    // Se chegou ao fim do segmento, avança para o próximo
    if (g_RandomFish.segment_t >= 1.0f) {
        g_RandomFish.segment_t -= 1.0f;  // Mantém o excesso para continuidade
        
        // Avança a fila de waypoints
        AdvanceWaypoints();
    }
    
    // Calcular posição usando Catmull-Rom spline
    // A curva passa suavemente por waypoint[1] e waypoint[2]
    // waypoint[0] e waypoint[3] são usados para calcular tangentes
    g_RandomFish.position = CatmullRom(
        g_RandomFish.segment_t,
        g_RandomFish.waypoints[0],
        g_RandomFish.waypoints[1],
        g_RandomFish.waypoints[2],
        g_RandomFish.waypoints[3]
    );
    
    // Garantir que a posição final também respeita os limites do grid
    g_RandomFish.position = ClampToGrid(g_RandomFish.position);
    
    // Calcular rotação baseada na direção do movimento
    glm::vec3 movement = g_RandomFish.position - old_position;
    if (glm::length(movement) > 0.001f) {
        g_RandomFish.rotation_y = atan2(movement.x, movement.z);
    }
}

// Tenta fisgar o peixe quando anzol está próximo
bool TryHookFish(const glm::vec3& hook_pos, float radius) {
    if (g_RandomFish.is_hooked) return true;
    
    float dist = glm::length(g_RandomFish.position - hook_pos);
    if (dist > radius + HOOK_RADIUS) return false;
    
    const FishTypeInfo& info = g_FishTypes[g_RandomFish.type];
    float roll = RandomFloat(0.0f, 1.0f);
    
    if (roll <= info.catch_chance) {
        g_RandomFish.is_hooked = true;
        printf("[Fish] %s fisgado! (+%d pontos)\n", info.name, info.points);
        return true;
    }
    
    printf("[Fish] %s escapou! (%.0f%% chance)\n", info.name, info.catch_chance * 100.0f);
    return false;
}

// =====================================================================
// Funções de acesso ao peixe aleatório (para collisions.cpp)
// =====================================================================

glm::vec3 GetRandomFishPosition() {
    return g_RandomFish.position;
}

const FishTypeInfo& GetRandomFishInfo() {
    return g_FishTypes[g_RandomFish.type];
}

bool IsRandomFishHooked() {
    return g_RandomFish.is_hooked;
}

void SetRandomFishHooked(bool hooked) {
    g_RandomFish.is_hooked = hooked;
}
