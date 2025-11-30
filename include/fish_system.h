#ifndef FISH_SYSTEM_H
#define FISH_SYSTEM_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

// =====================================================================
// Enumeração dos tipos de peixes disponíveis
// =====================================================================
enum FishType {
    FISH_COMMON_CARP,       // Carpa comum - muito comum, fácil de pescar
    FISH_GOLDEN_CARP,       // Carpa dourada - rara, difícil de pescar
    FISH_TILAPIA,           // Tilápia - comum, fácil
    FISH_BASS,              // Robalo - moderado
    FISH_CATFISH,           // Bagre - moderado
    FISH_RAINBOW_TROUT,     // Truta arco-íris - rara
    FISH_TYPE_COUNT         // Total de tipos
};

// =====================================================================
// Estrutura com informações de cada tipo de peixe
// =====================================================================
struct FishTypeInfo {
    FishType type;
    std::string name;           // Nome do peixe
    std::string model_file;     // Arquivo .obj do modelo
    float spawn_probability;    // Probabilidade de spawn (0.0 a 1.0)
    float catch_difficulty;     // Dificuldade de fisgar (0.0 = fácil, 1.0 = muito difícil)
    float min_speed;            // Velocidade mínima de natação
    float max_speed;            // Velocidade máxima de natação
    float base_points;          // Pontos base ao capturar
    glm::vec3 scale;            // Escala do modelo
    int texture_id;             // ID da textura (-1 se usar padrão)
    
    FishTypeInfo() 
        : type(FISH_COMMON_CARP), name("Unknown"), model_file(""), 
          spawn_probability(0.0f), catch_difficulty(0.5f),
          min_speed(0.3f), max_speed(0.8f), base_points(10.0f),
          scale(1.0f), texture_id(-1) {}
};

// =====================================================================
// Estrutura para um peixe ativo no mundo
// =====================================================================
struct ActiveFish {
    int id;                     // ID único do peixe
    FishType type;              // Tipo do peixe
    glm::vec3 position;         // Posição atual
    float rotation_y;           // Rotação (direção que está nadando)
    float speed;                // Velocidade atual
    
    // Curva de Bézier para movimento
    glm::vec3 bezier_points[4]; // Pontos de controle da curva
    float bezier_t;             // Parâmetro t da curva (0.0 a 1.0)
    
    // Estado do peixe
    bool is_active;             // Se está ativo no mundo
    bool is_hooked;             // Se foi fisgado
    bool is_fleeing;            // Se está fugindo da isca
    float interest_level;       // Nível de interesse na isca (0.0 a 1.0)
    
    // Área onde está nadando
    int current_area;           // Área atual do mapa
    
    ActiveFish() 
        : id(-1), type(FISH_COMMON_CARP), position(0.0f), rotation_y(0.0f),
          speed(0.5f), bezier_t(0.0f), is_active(false), is_hooked(false),
          is_fleeing(false), interest_level(0.0f), current_area(0) {}
};

// =====================================================================
// Estrutura para informações de renderização
// =====================================================================
struct FishRenderInfo {
    GLuint program_id;
    GLint model_uniform;
    GLint view_uniform;
    GLint projection_uniform;
    GLint object_id_uniform;
    GLint bbox_min_uniform;
    GLint bbox_max_uniform;
};

// =====================================================================
// Funções do sistema de peixes
// =====================================================================

// Inicialização e limpeza
void InitializeFishSystem();
void CleanupFishSystem();

// Carregamento de modelos
void LoadFishModels();
const FishTypeInfo& GetFishTypeInfo(FishType type);

// Spawn e gerenciamento de peixes
int SpawnFish(int area, FishType type = FISH_TYPE_COUNT);  // FISH_TYPE_COUNT = aleatório
int SpawnRandomFish(int area);
void DespawnFish(int fish_id);
void DespawnAllFish();

// Atualização
void UpdateFishSystem(float delta_time);
void UpdateFishMovement(ActiveFish& fish, float delta_time);
void UpdateFishBezierPath(ActiveFish& fish);

// Interação com isca
void UpdateFishBaitInteraction(ActiveFish& fish, const glm::vec3& bait_position, float delta_time);
bool TryHookFish(int fish_id, const glm::vec3& bait_position);
float CalculateCatchChance(const ActiveFish& fish, const glm::vec3& bait_position);

// Geração de rotas
void GenerateRandomBezierPath(ActiveFish& fish, int area);
glm::vec3 CalculateBezierPosition(const ActiveFish& fish);

// Consultas
int GetActiveFishCount();
ActiveFish* GetFishById(int fish_id);
std::vector<ActiveFish*> GetFishInArea(int area);
ActiveFish* GetNearestFishToBait(const glm::vec3& bait_position);

// Renderização (auxiliar)
void GetFishModelName(FishType type, std::string& out_name);

// Configurações
void SetMaxActiveFish(int max_fish);
void SetFishSpawnRate(float rate);

#endif // FISH_SYSTEM_H
