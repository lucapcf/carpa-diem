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
// Configurações da área de natação do peixe
// =====================================================================
static const float SWIM_DEPTH_MIN  = -1.5f;  // Profundidade mínima (mais fundo)
static const float SWIM_DEPTH_MAX  = -0.3f;  // Profundidade máxima (mais raso)

// Distância mínima e máxima para o próximo destino (a partir da posição atual)
static const float MIN_MOVE_DISTANCE = 1.0f;
static const float MAX_MOVE_DISTANCE = 3.0f;

// Limites do GRID (mapa vai de -MAP_SIZE/2 a +MAP_SIZE/2)
// MAP_SIZE = 20.0f, então limites são -10 a +10
// Adicionamos uma margem para o peixe não encostar na borda
static const float GRID_MARGIN = 1.0f;
static const float GRID_MIN_X = -10.0f + GRID_MARGIN;
static const float GRID_MAX_X =  10.0f - GRID_MARGIN;
static const float GRID_MIN_Z = -10.0f + GRID_MARGIN;
static const float GRID_MAX_Z =  10.0f - GRID_MARGIN;

// Distância mínima para considerar que chegou ao destino
static const float ARRIVAL_THRESHOLD = 0.3f;

// Flag para inicialização do gerador de números aleatórios
static bool g_RandomInitialized = false;

// Destino atual do peixe antigo (ponto para onde está nadando)
static glm::vec3 g_FishDestination;

// =====================================================================
// Peixe com movimento aleatório (novo sistema)
// =====================================================================
RandomFish g_RandomFish;

// =====================================================================
// Funções auxiliares
// =====================================================================

// Retorna um float aleatório entre min e max
static float RandomFloat(float min, float max) {
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    float normalized = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + normalized * (max - min);
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

void InitializeFishSystem() {
    printf("=================================================\n");
    printf("Inicializando sistema de peixes...\n");
    printf("=================================================\n");
    
    // Inicializa gerador de números aleatórios
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    
    // Valores iniciais (serão sobrescritos quando entrar no modo de pesca)
    g_Fish.position = glm::vec3(0.0f, -0.5f, 0.0f);
    g_Fish.speed = 0.8f;  // Velocidade de natação
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

const char* GetFishModelName() {
    return "fish_Cube";  // Nome do objeto no arquivo fish.obj
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
    
    // Pega o centro da área de pesca para posição inicial
    glm::vec3 area_center = g_MapAreas[area].center;
    
    // Posição inicial do peixe (clampada ao grid)
    glm::vec3 start_pos = ClampToGrid(glm::vec3(
        area_center.x + RandomFloat(-2.0f, 2.0f),
        RandomFloat(SWIM_DEPTH_MIN, SWIM_DEPTH_MAX),
        area_center.z + RandomFloat(-2.0f, 2.0f)
    ));
    
    // Inicializa todos os waypoints
    // waypoint[0] = ponto anterior (para cálculo de tangente)
    // waypoint[1] = posição atual do peixe
    // waypoint[2] = próximo destino
    // waypoint[3] = destino seguinte
    g_RandomFish.waypoints[0] = ClampToGrid(start_pos + glm::vec3(RandomFloat(-1.0f, 1.0f), 0.0f, RandomFloat(-1.0f, 1.0f)));
    g_RandomFish.waypoints[1] = start_pos;
    g_RandomFish.waypoints[2] = GenerateRandomPointNear(g_RandomFish.waypoints[1]);
    g_RandomFish.waypoints[3] = GenerateRandomPointNear(g_RandomFish.waypoints[2]);
    
    g_RandomFish.position = start_pos;
    g_RandomFish.segment_t = 0.0f;
    g_RandomFish.speed = RandomFloat(0.4f, 0.7f);  // Velocidade no segmento
    g_RandomFish.rotation_y = RandomFloat(0.0f, 2.0f * 3.14159f);
    
    printf("[RandomFish] Peixe gerado na area %d em (%.2f, %.2f, %.2f)\n", 
           area, g_RandomFish.position.x, g_RandomFish.position.y, g_RandomFish.position.z);
    printf("[RandomFish] Waypoints: [0](%.1f,%.1f) [1](%.1f,%.1f) [2](%.1f,%.1f) [3](%.1f,%.1f)\n",
           g_RandomFish.waypoints[0].x, g_RandomFish.waypoints[0].z,
           g_RandomFish.waypoints[1].x, g_RandomFish.waypoints[1].z,
           g_RandomFish.waypoints[2].x, g_RandomFish.waypoints[2].z,
           g_RandomFish.waypoints[3].x, g_RandomFish.waypoints[3].z);
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
    // (a curva Catmull-Rom pode ultrapassar os waypoints)
    g_RandomFish.position = ClampToGrid(g_RandomFish.position);
    
    // Calcular rotação baseada na direção do movimento
    glm::vec3 movement = g_RandomFish.position - old_position;
    if (glm::length(movement) > 0.001f) {
        g_RandomFish.rotation_y = atan2(movement.x, movement.z);
    }
}
