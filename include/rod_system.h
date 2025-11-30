#ifndef ROD_SYSTEM_H
#define ROD_SYSTEM_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glad/glad.h>

// Estrutura para passar informações de rendering para a linha de pesca
struct FishingLineRenderInfo {
    GLuint program_id;
    GLint model_uniform;
    GLint object_id_uniform;
    GLint bbox_min_uniform;
    GLint bbox_max_uniform;
};

// Função para inicializar e carregar os modelos das varas
void InitializeRodSystem();

// Funções para controle do arremesso
void StartChargingThrow();
float ReleaseThrow(); // Retorna a força calculada (0.0 se não estava carregando)
bool IsCharging();
float GetCurrentChargePercentage(); // Retorna 0.0 a 1.0 para UI

// Funções para a linha de pesca
void InitializeFishingLine();
void CleanupFishingLine();
void DrawFishingLine(const glm::vec3& rod_tip, const glm::vec3& bait_position, const FishingLineRenderInfo& render_info);

#endif // ROD_SYSTEM_H
