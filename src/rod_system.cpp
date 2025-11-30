#include "rod_system.h"
#include "game_types.h" // Para M_PI e M_PI_2
#include <cstdio>
#include <GLFW/glfw3.h> // Necessário para glfwGetTime()
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

// Incluir a estrutura ObjModel e funções de carregamento
#include <tiny_obj_loader.h>
#include <string>
#include <vector>
#include <stdexcept>

// Variáveis de estado do sistema de varas
static bool g_IsCharging = false;
static float g_ChargeStartTime = 0.0f;

// Constantes de força (Internas ao módulo)
static const float MIN_THROW_POWER = 5.0f;
static const float MAX_THROW_POWER = 30.0f;
static const float MAX_CHARGE_TIME = 2.0f; // Segundos para carga máxima

// Variáveis para a linha de pesca (internas ao módulo)
static GLuint g_LineVAO = 0;
static GLuint g_LineVBO = 0;

// ID do objeto para linha de pesca no shader
static const int FISHING_LINE_OBJECT_ID = 6;

// Função auxiliar para criar matriz identidade (evita dependência de matrices.h)
static glm::mat4 IdentityMatrix() {
    return glm::mat4(1.0f);
}

// Estrutura para carregar modelos .obj (copiada da main.cpp)
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

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
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declarações das funções que já existem na main.cpp
extern void ComputeNormals(ObjModel* model);
extern void BuildTrianglesAndAddToVirtualScene(ObjModel* model);

void InitializeRodSystem() {
    printf("=================================================\n");
    printf("Inicializando sistema de varas de pesca...\n");
    printf("=================================================\n");
    
    // Carregar modelo 3D da vara básica
    try {
        ObjModel fishing_pole("../../data/models/fishing_pole_01.obj");
        ComputeNormals(&fishing_pole);
        BuildTrianglesAndAddToVirtualScene(&fishing_pole);
        printf("Vara básica (fishing_pole_01) carregada com sucesso!\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "ERRO ao carregar fishing_pole_01: %s\n", e.what());
    }
    
    // Carregar modelo 3D da vara avançada (fishing_rod)
    // AINDA NÂO IMPLEMENTADO TROCA ENTRE VARAS
    // Descomente quando o arquivo estiver corrigido
    /*
    try {
        ObjModel fishing_rod("../../data/models/fishing_rod.obj");
        ComputeNormals(&fishing_rod);
        BuildTrianglesAndAddToVirtualScene(&fishing_rod);
        printf("Vara avançada (fishing_rod) carregada com sucesso!\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "ERRO ao carregar fishing_rod: %s\n", e.what());
    }
    */
    
    printf("=================================================\n");
    printf("Sistema de varas inicializado!\n");
    printf("=================================================\n");
}

void StartChargingThrow() {
    if (!g_IsCharging) {
        g_IsCharging = true;
        g_ChargeStartTime = (float)glfwGetTime();
        printf("Carregando lancamento... (Segure para aumentar a forca)\n");
    }
}

float ReleaseThrow() {
    if (!g_IsCharging) return 0.0f;

    g_IsCharging = false;
    
    float current_time = (float)glfwGetTime();
    float charge_duration = current_time - g_ChargeStartTime;
    
    // Limitar o tempo ao máximo definido
    if (charge_duration > MAX_CHARGE_TIME) charge_duration = MAX_CHARGE_TIME;
    
    // Interpolação linear: (tempo / tempo_max) vai de 0.0 a 1.0
    float power_factor = charge_duration / MAX_CHARGE_TIME;
    
    // Força final = Mínimo + (Diferença * Fator)
    float final_power = MIN_THROW_POWER + (power_factor * (MAX_THROW_POWER - MIN_THROW_POWER));
    
    printf("Arremesso liberado! Forca: %.2f (Carga: %.0f%%)\n", final_power, power_factor * 100.0f);
    
    return final_power;
}

bool IsCharging() {
    return g_IsCharging;
}

float GetCurrentChargePercentage() {
    if (!g_IsCharging) return 0.0f;
    
    float current_time = (float)glfwGetTime();
    float charge_duration = current_time - g_ChargeStartTime;
    
    if (charge_duration > MAX_CHARGE_TIME) charge_duration = MAX_CHARGE_TIME;
    
    return charge_duration / MAX_CHARGE_TIME;
}

// =====================================================================
// Funções da linha de pesca
// =====================================================================

void InitializeFishingLine() {
    if (g_LineVAO == 0) {
        glGenVertexArrays(1, &g_LineVAO);
        glGenBuffers(1, &g_LineVBO);
        printf("Linha de pesca inicializada (VAO: %u, VBO: %u)\n", g_LineVAO, g_LineVBO);
    }
}

void CleanupFishingLine() {
    if (g_LineVBO != 0) {
        glDeleteBuffers(1, &g_LineVBO);
        g_LineVBO = 0;
    }
    if (g_LineVAO != 0) {
        glDeleteVertexArrays(1, &g_LineVAO);
        g_LineVAO = 0;
    }
}

void DrawFishingLine(const glm::vec3& rod_tip, const glm::vec3& bait_position, const FishingLineRenderInfo& render_info) {
    // Inicializar VAO/VBO se necessário
    if (g_LineVAO == 0) {
        InitializeFishingLine();
    }
    
    // Criar vértices da linha: ponto inicial (ponta da vara) -> ponto final (isca)
    float line_vertices[] = {
        rod_tip.x, rod_tip.y, rod_tip.z,
        bait_position.x, bait_position.y, bait_position.z
    };
    
    // Atualizar buffer com as novas posições
    glBindVertexArray(g_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Salvar programa atual e usar o programa fornecido
    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    glUseProgram(render_info.program_id);
    
    // Matriz model identidade (a linha já está em coordenadas do mundo)
    glm::mat4 model = IdentityMatrix();
    glUniformMatrix4fv(render_info.model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    
    // Usar ID definido no shader para linha branca
    glUniform1i(render_info.object_id_uniform, FISHING_LINE_OBJECT_ID);
    
    // Bounding box vazia (não é usada para linhas)
    glUniform4f(render_info.bbox_min_uniform, 0.0f, 0.0f, 0.0f, 1.0f);
    glUniform4f(render_info.bbox_max_uniform, 1.0f, 1.0f, 1.0f, 1.0f);
    
    // Desenhar a linha
    glDrawArrays(GL_LINES, 0, 2);
    
    // Restaurar estado
    glBindVertexArray(0);
    glUseProgram(current_program);
}
