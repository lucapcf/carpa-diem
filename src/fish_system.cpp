#include "fish_system.h"
#include "game_types.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

// Incluir para carregar modelos
#include <tiny_obj_loader.h>
#include <stdexcept>

// =====================================================================
// Variáveis internas do módulo
// =====================================================================

// Informações de cada tipo de peixe
static FishTypeInfo g_FishTypes[FISH_TYPE_COUNT];

// Lista de peixes ativos no mundo
static std::vector<ActiveFish> g_ActiveFish;
static int g_NextFishId = 0;

// Configurações do sistema
static int g_MaxActiveFish = 10;
static float g_FishSpawnRate = 1.0f;  // Multiplicador de spawn
static float g_LastSpawnTime = 0.0f;
static float g_SpawnInterval = 5.0f;  // Segundos entre tentativas de spawn

// Flag de inicialização
static bool g_FishSystemInitialized = false;

// Constantes do mapa (referência de game_types.h)
extern const float MAP_SCALE;
extern const float MAP_SIZE;
extern const float AREA_WIDTH;
extern const float AREA_HEIGHT;

// =====================================================================
// Estrutura para carregar modelos .obj (mesma estrutura de main.cpp)
// =====================================================================
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo de peixe \"%s\"...\n", filename);

        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo de peixe.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr, "Aviso: Objeto sem nome no arquivo '%s'.\n", filename);
            }
            else
            {
                printf("- Objeto '%s'\n", shapes[shape].name.c_str());
            }
        }

        printf("OK.\n");
    }
};

// Declarações externas das funções de main.cpp
extern void ComputeNormals(ObjModel* model);
extern void BuildTrianglesAndAddToVirtualScene(ObjModel* model);

// =====================================================================
// Funções auxiliares internas
// =====================================================================

// Gera número aleatório entre 0.0 e 1.0
static float RandomFloat() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

// Gera número aleatório entre min e max
static float RandomRange(float min_val, float max_val) {
    return min_val + RandomFloat() * (max_val - min_val);
}

// Calcula ponto na curva de Bézier cúbica
static glm::vec3 CubicBezier(float t, const glm::vec3& p0, const glm::vec3& p1, 
                              const glm::vec3& p2, const glm::vec3& p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    glm::vec3 point = uuu * p0;           // (1-t)^3 * P0
    point += 3.0f * uu * t * p1;          // 3(1-t)^2 * t * P1
    point += 3.0f * u * tt * p2;          // 3(1-t) * t^2 * P2
    point += ttt * p3;                     // t^3 * P3
    
    return point;
}

// Obtém os limites de uma área do mapa
static void GetAreaBounds(int area, glm::vec3& out_min, glm::vec3& out_max) {
    // Mapa dividido em grade 3x2 (3 colunas, 2 linhas)
    // Área vai de 0 a 5: 
    //   0, 1, 2 (linha superior)
    //   3, 4, 5 (linha inferior)
    
    float half_map = MAP_SIZE / 2.0f;
    
    int col = area % 3;
    int row = area / 3;
    
    float area_width = MAP_SIZE / 3.0f;
    float area_height = MAP_SIZE / 2.0f;
    
    out_min.x = -half_map + col * area_width;
    out_min.z = -half_map + row * area_height;
    out_min.y = -0.5f;  // Profundidade do peixe na água
    
    out_max.x = out_min.x + area_width;
    out_max.z = out_min.z + area_height;
    out_max.y = -0.3f;
}

// =====================================================================
// Inicialização e limpeza
// =====================================================================

void InitializeFishSystem() {
    if (g_FishSystemInitialized) {
        printf("Sistema de peixes já inicializado.\n");
        return;
    }
    
    printf("=================================================\n");
    printf("Inicializando sistema de peixes...\n");
    printf("=================================================\n");
    
    // Inicializar gerador de números aleatórios
    srand(static_cast<unsigned int>(time(NULL)));
    
    // Configurar informações de cada tipo de peixe
    // CARPA COMUM - Muito comum, fácil de pescar
    g_FishTypes[FISH_COMMON_CARP].type = FISH_COMMON_CARP;
    g_FishTypes[FISH_COMMON_CARP].name = "Carpa Comum";
    g_FishTypes[FISH_COMMON_CARP].model_file = "../../data/fish.obj";  // Modelo padrão
    g_FishTypes[FISH_COMMON_CARP].spawn_probability = 0.35f;
    g_FishTypes[FISH_COMMON_CARP].catch_difficulty = 0.2f;
    g_FishTypes[FISH_COMMON_CARP].min_speed = 0.3f;
    g_FishTypes[FISH_COMMON_CARP].max_speed = 0.6f;
    g_FishTypes[FISH_COMMON_CARP].base_points = 10.0f;
    g_FishTypes[FISH_COMMON_CARP].scale = glm::vec3(0.5f);
    g_FishTypes[FISH_COMMON_CARP].texture_id = -1;
    
    // CARPA DOURADA - Rara, difícil
    g_FishTypes[FISH_GOLDEN_CARP].type = FISH_GOLDEN_CARP;
    g_FishTypes[FISH_GOLDEN_CARP].name = "Carpa Dourada";
    g_FishTypes[FISH_GOLDEN_CARP].model_file = "../../data/fish.obj";
    g_FishTypes[FISH_GOLDEN_CARP].spawn_probability = 0.05f;
    g_FishTypes[FISH_GOLDEN_CARP].catch_difficulty = 0.8f;
    g_FishTypes[FISH_GOLDEN_CARP].min_speed = 0.5f;
    g_FishTypes[FISH_GOLDEN_CARP].max_speed = 1.0f;
    g_FishTypes[FISH_GOLDEN_CARP].base_points = 100.0f;
    g_FishTypes[FISH_GOLDEN_CARP].scale = glm::vec3(0.6f);
    g_FishTypes[FISH_GOLDEN_CARP].texture_id = -1;
    
    // TILÁPIA - Comum, fácil
    g_FishTypes[FISH_TILAPIA].type = FISH_TILAPIA;
    g_FishTypes[FISH_TILAPIA].name = "Tilápia";
    g_FishTypes[FISH_TILAPIA].model_file = "../../data/fish.obj";
    g_FishTypes[FISH_TILAPIA].spawn_probability = 0.30f;
    g_FishTypes[FISH_TILAPIA].catch_difficulty = 0.25f;
    g_FishTypes[FISH_TILAPIA].min_speed = 0.4f;
    g_FishTypes[FISH_TILAPIA].max_speed = 0.7f;
    g_FishTypes[FISH_TILAPIA].base_points = 15.0f;
    g_FishTypes[FISH_TILAPIA].scale = glm::vec3(0.4f);
    g_FishTypes[FISH_TILAPIA].texture_id = -1;
    
    // ROBALO - Moderado
    g_FishTypes[FISH_BASS].type = FISH_BASS;
    g_FishTypes[FISH_BASS].name = "Robalo";
    g_FishTypes[FISH_BASS].model_file = "../../data/fish.obj";
    g_FishTypes[FISH_BASS].spawn_probability = 0.15f;
    g_FishTypes[FISH_BASS].catch_difficulty = 0.5f;
    g_FishTypes[FISH_BASS].min_speed = 0.5f;
    g_FishTypes[FISH_BASS].max_speed = 0.9f;
    g_FishTypes[FISH_BASS].base_points = 35.0f;
    g_FishTypes[FISH_BASS].scale = glm::vec3(0.55f);
    g_FishTypes[FISH_BASS].texture_id = -1;
    
    // BAGRE - Moderado, mais lento
    g_FishTypes[FISH_CATFISH].type = FISH_CATFISH;
    g_FishTypes[FISH_CATFISH].name = "Bagre";
    g_FishTypes[FISH_CATFISH].model_file = "../../data/fish.obj";
    g_FishTypes[FISH_CATFISH].spawn_probability = 0.10f;
    g_FishTypes[FISH_CATFISH].catch_difficulty = 0.4f;
    g_FishTypes[FISH_CATFISH].min_speed = 0.2f;
    g_FishTypes[FISH_CATFISH].max_speed = 0.5f;
    g_FishTypes[FISH_CATFISH].base_points = 25.0f;
    g_FishTypes[FISH_CATFISH].scale = glm::vec3(0.65f);
    g_FishTypes[FISH_CATFISH].texture_id = -1;
    
    // TRUTA ARCO-ÍRIS - Rara
    g_FishTypes[FISH_RAINBOW_TROUT].type = FISH_RAINBOW_TROUT;
    g_FishTypes[FISH_RAINBOW_TROUT].name = "Truta Arco-Íris";
    g_FishTypes[FISH_RAINBOW_TROUT].model_file = "../../data/fish.obj";
    g_FishTypes[FISH_RAINBOW_TROUT].spawn_probability = 0.05f;
    g_FishTypes[FISH_RAINBOW_TROUT].catch_difficulty = 0.7f;
    g_FishTypes[FISH_RAINBOW_TROUT].min_speed = 0.6f;
    g_FishTypes[FISH_RAINBOW_TROUT].max_speed = 1.1f;
    g_FishTypes[FISH_RAINBOW_TROUT].base_points = 75.0f;
    g_FishTypes[FISH_RAINBOW_TROUT].scale = glm::vec3(0.5f);
    g_FishTypes[FISH_RAINBOW_TROUT].texture_id = -1;
    
    // Reservar espaço para peixes ativos
    g_ActiveFish.reserve(g_MaxActiveFish);
    
    g_FishSystemInitialized = true;
    printf("Sistema de peixes inicializado com sucesso!\n");
    printf("Tipos de peixes configurados: %d\n", FISH_TYPE_COUNT);
}

void CleanupFishSystem() {
    DespawnAllFish();
    g_ActiveFish.clear();
    g_FishSystemInitialized = false;
    printf("Sistema de peixes finalizado.\n");
}

// =====================================================================
// Carregamento de modelos
// =====================================================================

void LoadFishModels() {
    printf("Carregando modelos de peixes...\n");
    
    // Por enquanto, todos usam o mesmo modelo base
    // Quando tiver modelos diferentes, carregar cada um aqui
    // Os modelos já são carregados na main.cpp por enquanto
    
    printf("Modelos de peixes prontos.\n");
}

const FishTypeInfo& GetFishTypeInfo(FishType type) {
    if (type >= 0 && type < FISH_TYPE_COUNT) {
        return g_FishTypes[type];
    }
    return g_FishTypes[FISH_COMMON_CARP];  // Retorna carpa comum como fallback
}

// =====================================================================
// Spawn e gerenciamento
// =====================================================================

int SpawnFish(int area, FishType type) {
    if (!g_FishSystemInitialized) {
        printf("Erro: Sistema de peixes não inicializado!\n");
        return -1;
    }
    
    if (static_cast<int>(g_ActiveFish.size()) >= g_MaxActiveFish) {
        return -1;  // Limite atingido
    }
    
    // Se tipo não especificado, escolher aleatoriamente baseado em probabilidades
    if (type == FISH_TYPE_COUNT) {
        type = static_cast<FishType>(SpawnRandomFish(area));
        if (type == FISH_TYPE_COUNT) {
            return -1;
        }
        // SpawnRandomFish já criou o peixe, retornar o último ID
        return g_ActiveFish.back().id;
    }
    
    ActiveFish fish;
    fish.id = g_NextFishId++;
    fish.type = type;
    fish.is_active = true;
    fish.is_hooked = false;
    fish.is_fleeing = false;
    fish.interest_level = 0.0f;
    fish.current_area = area;
    
    // Configurar velocidade baseada no tipo
    const FishTypeInfo& info = g_FishTypes[type];
    fish.speed = RandomRange(info.min_speed, info.max_speed);
    
    // Gerar rota de Bézier aleatória na área
    GenerateRandomBezierPath(fish, area);
    fish.bezier_t = RandomFloat();  // Começar em ponto aleatório da curva
    fish.position = CalculateBezierPosition(fish);
    
    g_ActiveFish.push_back(fish);
    
    printf("Peixe spawned: %s (ID: %d) na área %d\n", 
           info.name.c_str(), fish.id, area);
    
    return fish.id;
}

int SpawnRandomFish(int area) {
    // Escolher tipo baseado em probabilidades
    float roll = RandomFloat();
    float cumulative = 0.0f;
    FishType selected_type = FISH_COMMON_CARP;
    
    for (int i = 0; i < FISH_TYPE_COUNT; i++) {
        cumulative += g_FishTypes[i].spawn_probability;
        if (roll <= cumulative) {
            selected_type = static_cast<FishType>(i);
            break;
        }
    }
    
    // Criar o peixe diretamente aqui para evitar recursão
    if (static_cast<int>(g_ActiveFish.size()) >= g_MaxActiveFish) {
        return -1;
    }
    
    ActiveFish fish;
    fish.id = g_NextFishId++;
    fish.type = selected_type;
    fish.is_active = true;
    fish.is_hooked = false;
    fish.is_fleeing = false;
    fish.interest_level = 0.0f;
    fish.current_area = area;
    
    const FishTypeInfo& info = g_FishTypes[selected_type];
    fish.speed = RandomRange(info.min_speed, info.max_speed);
    
    GenerateRandomBezierPath(fish, area);
    fish.bezier_t = RandomFloat();
    fish.position = CalculateBezierPosition(fish);
    
    g_ActiveFish.push_back(fish);
    
    printf("Peixe aleatório spawned: %s (ID: %d) na área %d\n", 
           info.name.c_str(), fish.id, area);
    
    return fish.id;
}

void DespawnFish(int fish_id) {
    for (auto it = g_ActiveFish.begin(); it != g_ActiveFish.end(); ++it) {
        if (it->id == fish_id) {
            printf("Peixe despawned: ID %d\n", fish_id);
            g_ActiveFish.erase(it);
            return;
        }
    }
}

void DespawnAllFish() {
    g_ActiveFish.clear();
    printf("Todos os peixes removidos.\n");
}

// =====================================================================
// Atualização
// =====================================================================

void UpdateFishSystem(float delta_time) {
    if (!g_FishSystemInitialized) return;
    
    // Atualizar cada peixe ativo
    for (auto& fish : g_ActiveFish) {
        if (fish.is_active && !fish.is_hooked) {
            UpdateFishMovement(fish, delta_time);
        }
    }
    
    // Tentar spawn de novos peixes periodicamente
    float current_time = static_cast<float>(glfwGetTime());
    if (current_time - g_LastSpawnTime > g_SpawnInterval / g_FishSpawnRate) {
        if (static_cast<int>(g_ActiveFish.size()) < g_MaxActiveFish) {
            // Spawn em área aleatória
            int random_area = rand() % 6;  // 6 áreas
            SpawnRandomFish(random_area);
        }
        g_LastSpawnTime = current_time;
    }
}

void UpdateFishMovement(ActiveFish& fish, float delta_time) {
    // Guardar posição anterior para calcular direção
    glm::vec3 old_position = fish.position;
    
    // Avançar na curva de Bézier
    fish.bezier_t += fish.speed * delta_time;
    
    // Se chegou ao fim da curva, gerar nova rota
    if (fish.bezier_t >= 1.0f) {
        fish.bezier_t = 0.0f;
        UpdateFishBezierPath(fish);
    }
    
    // Calcular nova posição
    fish.position = CalculateBezierPosition(fish);
    
    // Calcular rotação baseada na direção do movimento
    glm::vec3 movement = fish.position - old_position;
    if (glm::length(movement) > 0.001f) {
        fish.rotation_y = atan2(movement.x, movement.z);
    }
}

void UpdateFishBezierPath(ActiveFish& fish) {
    // Gerar nova rota mantendo continuidade (último ponto vira primeiro)
    glm::vec3 start = fish.bezier_points[3];  // Fim da curva atual
    
    glm::vec3 area_min, area_max;
    GetAreaBounds(fish.current_area, area_min, area_max);
    
    // Chance de mudar de área
    if (RandomFloat() < 0.2f) {
        fish.current_area = rand() % 6;
        GetAreaBounds(fish.current_area, area_min, area_max);
    }
    
    fish.bezier_points[0] = start;
    
    // Gerar pontos de controle dentro da área
    for (int i = 1; i < 4; i++) {
        fish.bezier_points[i].x = RandomRange(area_min.x + 1.0f, area_max.x - 1.0f);
        fish.bezier_points[i].y = RandomRange(area_min.y, area_max.y);
        fish.bezier_points[i].z = RandomRange(area_min.z + 1.0f, area_max.z - 1.0f);
    }
}

// =====================================================================
// Interação com isca
// =====================================================================

void UpdateFishBaitInteraction(ActiveFish& fish, const glm::vec3& bait_position, float delta_time) {
    if (fish.is_hooked) return;
    
    // Calcular distância até a isca
    float distance = glm::length(fish.position - bait_position);
    
    // Raio de detecção da isca
    const float detection_radius = 5.0f;
    const float attraction_radius = 2.0f;
    
    if (distance < detection_radius) {
        // Peixe detectou a isca
        const FishTypeInfo& info = g_FishTypes[fish.type];
        
        // Aumentar interesse gradualmente (peixes difíceis demoram mais)
        float interest_rate = (1.0f - info.catch_difficulty) * delta_time;
        fish.interest_level = std::min(1.0f, fish.interest_level + interest_rate);
        
        if (distance < attraction_radius && fish.interest_level > 0.5f) {
            // Peixe interessado o suficiente - mover em direção à isca
            glm::vec3 direction = glm::normalize(bait_position - fish.position);
            fish.position += direction * fish.speed * 0.5f * delta_time;
            
            // Atualizar rotação para olhar para a isca
            fish.rotation_y = atan2(direction.x, direction.z);
        }
    } else {
        // Peixe longe da isca - perder interesse gradualmente
        fish.interest_level = std::max(0.0f, fish.interest_level - 0.1f * delta_time);
    }
}

bool TryHookFish(int fish_id, const glm::vec3& bait_position) {
    ActiveFish* fish = GetFishById(fish_id);
    if (!fish || fish->is_hooked) return false;
    
    float catch_chance = CalculateCatchChance(*fish, bait_position);
    float roll = RandomFloat();
    
    printf("Tentando fisgar %s - Chance: %.1f%%, Roll: %.2f\n",
           g_FishTypes[fish->type].name.c_str(), catch_chance * 100.0f, roll);
    
    if (roll < catch_chance) {
        fish->is_hooked = true;
        printf("PEIXE FISGADO! %s\n", g_FishTypes[fish->type].name.c_str());
        return true;
    }
    
    // Falhou - peixe foge
    fish->is_fleeing = true;
    fish->interest_level = 0.0f;
    fish->speed *= 1.5f;  // Acelera para fugir
    
    printf("Peixe escapou e está fugindo!\n");
    return false;
}

float CalculateCatchChance(const ActiveFish& fish, const glm::vec3& bait_position) {
    const FishTypeInfo& info = g_FishTypes[fish.type];
    
    // Base chance começa em 50%
    float base_chance = 0.5f;
    
    // Dificuldade do peixe reduz chance
    float difficulty_modifier = 1.0f - info.catch_difficulty;
    
    // Interesse do peixe aumenta chance
    float interest_modifier = fish.interest_level;
    
    // Distância afeta chance (mais perto = melhor)
    float distance = glm::length(fish.position - bait_position);
    float distance_modifier = 1.0f - std::min(1.0f, distance / 2.0f);
    
    // Calcular chance final
    float final_chance = base_chance * difficulty_modifier * (0.5f + interest_modifier * 0.5f) * (0.5f + distance_modifier * 0.5f);
    
    return std::max(0.05f, std::min(0.95f, final_chance));  // Clamp entre 5% e 95%
}

// =====================================================================
// Geração de rotas
// =====================================================================

void GenerateRandomBezierPath(ActiveFish& fish, int area) {
    glm::vec3 area_min, area_max;
    GetAreaBounds(area, area_min, area_max);
    
    // Gerar 4 pontos de controle dentro da área
    for (int i = 0; i < 4; i++) {
        fish.bezier_points[i].x = RandomRange(area_min.x + 1.0f, area_max.x - 1.0f);
        fish.bezier_points[i].y = RandomRange(area_min.y, area_max.y);
        fish.bezier_points[i].z = RandomRange(area_min.z + 1.0f, area_max.z - 1.0f);
    }
}

glm::vec3 CalculateBezierPosition(const ActiveFish& fish) {
    return CubicBezier(fish.bezier_t, 
                       fish.bezier_points[0], 
                       fish.bezier_points[1],
                       fish.bezier_points[2], 
                       fish.bezier_points[3]);
}

// =====================================================================
// Consultas
// =====================================================================

int GetActiveFishCount() {
    return static_cast<int>(g_ActiveFish.size());
}

ActiveFish* GetFishById(int fish_id) {
    for (auto& fish : g_ActiveFish) {
        if (fish.id == fish_id) {
            return &fish;
        }
    }
    return nullptr;
}

std::vector<ActiveFish*> GetFishInArea(int area) {
    std::vector<ActiveFish*> result;
    for (auto& fish : g_ActiveFish) {
        if (fish.current_area == area && fish.is_active) {
            result.push_back(&fish);
        }
    }
    return result;
}

ActiveFish* GetNearestFishToBait(const glm::vec3& bait_position) {
    ActiveFish* nearest = nullptr;
    float min_distance = FLT_MAX;
    
    for (auto& fish : g_ActiveFish) {
        if (!fish.is_active || fish.is_hooked) continue;
        
        float distance = glm::length(fish.position - bait_position);
        if (distance < min_distance) {
            min_distance = distance;
            nearest = &fish;
        }
    }
    
    return nearest;
}

// =====================================================================
// Renderização auxiliar
// =====================================================================

void GetFishModelName(FishType type, std::string& out_name) {
    // Por enquanto todos usam "the_fish" que é o nome do objeto no fish.obj
    out_name = "the_fish";
}

// =====================================================================
// Configurações
// =====================================================================

void SetMaxActiveFish(int max_fish) {
    g_MaxActiveFish = std::max(1, max_fish);
    g_ActiveFish.reserve(g_MaxActiveFish);
}

void SetFishSpawnRate(float rate) {
    g_FishSpawnRate = std::max(0.1f, rate);
}
