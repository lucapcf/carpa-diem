//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "game_state.h"
#include "game_types.h"
#include "rod_system.h"
#include "fish_system.h"
#include "collisions.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
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


// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Função para desenhar grid das zonas de pesca
void DrawFishingGrid();

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
    glm::vec3    material_kd; // Cor difusa do material (do arquivo .mtl)
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse
// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_fish_texture_id_uniform; // ID da textura do peixe atual
GLint g_material_kd_uniform;

// Grid variables para desenhar linhas das zonas de pesca
GLuint g_GridVAO = 0;
GLuint g_GridVBO = 0;
int g_GridVertexCount = 0;
bool g_ShowGrid = true;  // Toggle para mostrar/ocultar o grid

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Variáveis auxiliares para controle do mouse na fase de pesca
double g_FishingLastCursorX = 0.0;
double g_FishingLastCursorY = 0.0;
bool g_FishingFirstMouse = true;

glm::vec4 camera_view_vector;

// Funções de inicialização e renderização
GLFWwindow* InitializeWindow();
void SetupCallbacks(GLFWwindow* window);
void InitializeOpenGL();
void LoadGameResources();
void UpdateCameras(glm::mat4& view, glm::vec4& camera_position, glm::mat4& projection);
void RenderScene(GLFWwindow* window, const glm::mat4& view, const glm::mat4& projection);

// Variáveis para debug da posição da vara
glm::vec3 g_RodDebugOffset = glm::vec3(-0.250f, -0.220f, 0.320f);
float g_RodDebugStep = 0.01f;

// Variáveis para debug da ponta da vara (onde a linha começa)
glm::vec4 g_RodTipDebug = glm::vec4(4.0f, 41.0f, 4.0f, 1.0f);
float g_RodTipDebugStep = 1.0f;

// =====================================================================
// Funções auxiliares do jogo
// =====================================================================

// Constantes para a linha de pesca
const glm::vec3 g_RodOffset = glm::vec3(-0.250f, -0.220f, 0.320f);

// Função para calcular a posição da ponta da vara no mundo
glm::vec3 GetRodTipPosition() {
    float corrected_rotation = g_Boat.rotation_y + g_CameraTheta;
    
    // Replicar a matriz de transformação da vara usada no render loop
    glm::mat4 model = Matrix_Translate(g_Boat.position.x, g_Boat.position.y + 1.0f, g_Boat.position.z)
          * Matrix_Rotate_Y(corrected_rotation)
          * Matrix_Translate(g_RodOffset.x, g_RodOffset.y, g_RodOffset.z)
          * Matrix_Rotate_Y(-M_PI_2)
          * Matrix_Rotate_Z(-M_PI / 6.0f)
          * Matrix_Scale(0.08f, 0.08f, 0.08f);
          
    glm::vec4 tip_world = model * g_RodTipDebug;  // Usando variável de debug
    return glm::vec3(tip_world);
}

// Função para inicializar o grid das zonas de pesca
void InitializeFishingGrid() {
    // Criar linhas para dividir o mapa em uma grade 3x2 (3 colunas, 2 linhas)
    // O mapa renderizado vai de -MAP_SIZE/2 a +MAP_SIZE/2 em ambos os eixos
    std::vector<float> grid_vertices;
    float half_map = MAP_SIZE / 2.0f;
    float col_width = MAP_SIZE / 3.0f;
    
    // Linhas verticais (dividem as 3 colunas)
    for (int i = 1; i < 3; i++) {
        float x = -half_map + col_width * i;
        // Linha vertical de z = -half_map até z = +half_map
        grid_vertices.push_back(x);     // x
        grid_vertices.push_back(-0.59f); // y (ligeiramente acima do mapa)
        grid_vertices.push_back(-half_map); // z (início)
        
        grid_vertices.push_back(x);     // x
        grid_vertices.push_back(-0.59f); // y
        grid_vertices.push_back(half_map);  // z (fim)
    }
    
    // Linha horizontal (divide as 2 linhas)
    float z = 0.0f; // Centro do mapa
    grid_vertices.push_back(-half_map); // x (início)
    grid_vertices.push_back(-0.59f);    // y
    grid_vertices.push_back(z);         // z
    
    grid_vertices.push_back(half_map);  // x (fim)
    grid_vertices.push_back(-0.59f);    // y
    grid_vertices.push_back(z);         // z
    
    g_GridVertexCount = grid_vertices.size() / 3;
    
    // Criar VAO e VBO para o grid
    glGenVertexArrays(1, &g_GridVAO);
    glGenBuffers(1, &g_GridVBO);
    
    glBindVertexArray(g_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), grid_vertices.data(), GL_STATIC_DRAW);
    
    // Atributo de posição (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

// Função para desenhar o grid das zonas de pesca
void DrawFishingGrid() {
    if (!g_ShowGrid || g_GridVAO == 0) return;
    
    // Salvar o estado atual
    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    
    // Usar o programa de shader atual
    glUseProgram(g_GpuProgramID);
    
    // Matriz model para o grid (sem transformações)
    glm::mat4 model = Matrix_Identity();
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    
    // Usar um object_id específico para o grid (pode usar um valor que não conflite)
    glUniform1i(g_object_id_uniform, 10); // Valor diferente dos outros objetos
    
    // Configurar bounding box vazia para o grid
    glUniform4f(g_bbox_min_uniform, 0.0f, 0.0f, 0.0f, 1.0f);
    glUniform4f(g_bbox_max_uniform, 1.0f, 1.0f, 1.0f, 1.0f);
    
    // Desenhar as linhas
    glBindVertexArray(g_GridVAO);
    glDrawArrays(GL_LINES, 0, g_GridVertexCount);
    glBindVertexArray(0);
    
    // Restaurar o programa anterior
    glUseProgram(current_program);
}

// Função para obter qual área o barco está atualmente
MapArea GetCurrentArea(glm::vec3 position) {
    for (int i = 0; i < NUM_AREAS; i++) {
        const MapAreaInfo& area = g_MapAreas[i];
        if (position.x >= area.min_bounds.x && position.x <= area.max_bounds.x &&
            position.z >= area.min_bounds.z && position.z <= area.max_bounds.z) {
            return static_cast<MapArea>(i);
        }
    }
    return AREA_1_1; // Default
}

// Função para calcular ponto na curva de Bézier cúbica
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    glm::vec3 point = uuu * p0;      // (1-t)^3 * P0
    point += 3 * uu * t * p1;        // 3(1-t)^2 * t * P1
    point += 3 * u * tt * p2;        // 3(1-t) * t^2 * P2
    point += ttt * p3;               // t^3 * P3
    
    return point;
}

// Função para configurar câmera top-down (Fase Navegação)
void SetupTopDownCamera(glm::mat4& view, glm::vec4& camera_position) {
    camera_position = glm::vec4(0.0f, 40.0f, 0.0f, 1.0f);
    glm::vec4 camera_view_vector = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    glm::vec4 camera_up_vector = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    
    view = Matrix_Camera_View(camera_position, camera_view_vector, camera_up_vector);
}

// Função auxiliar para resetar e capturar o mouse ao entrar na fase de pesca
void FirstPersonCameraConfig (GLFWwindow* window) {
    // Desabilitar cursor (captura e esconde)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Obter centro da janela e reposicionar cursor
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glfwSetCursorPos(window, width / 2.0, height / 2.0);

    // Resetar variáveis de rastreamento do mouse
    g_FishingFirstMouse = true;
    g_FishingLastCursorX = 0.0;
    g_FishingLastCursorY = 0.0;

    // Orientacao da camera para ficar para a frente do barco
    g_CameraTheta = M_PI;  
    g_CameraPhi = 0.0f;    
}

// Função para configurar câmera primeira pessoa (Fase de Pesca)
void SetupFirstPersonCamera(glm::mat4& view, glm::vec4& camera_position) {
    float corrected_rotation = g_Boat.rotation_y + g_CameraTheta; 
    
    float look_x = sin(corrected_rotation) * cos(g_CameraPhi);
    float look_y = -sin(g_CameraPhi);
    float look_z = cos(corrected_rotation) * cos(g_CameraPhi);
    
    glm::vec4 camera_position_c = glm::vec4(g_Boat.position.x, g_Boat.position.y + 1.0f, g_Boat.position.z, 1.0f);
    
    camera_view_vector = glm::vec4(look_x, look_y, look_z, 0.0f);
    glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    
    view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
    camera_position = camera_position_c;
}

// Função para configurar câmera livre (Debug)
void SetupDebugCamera(glm::mat4& view, glm::vec4& camera_position) {
    // Câmera livre em coordenadas esféricas
    float r = g_CameraDistance;
    float y = r * sin(g_CameraPhi);
    float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
    float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);
    
    camera_position = glm::vec4(x, y, z, 1.0f);
    glm::vec4 camera_view_vector = glm::vec4(-x, -y, -z, 0.0f); // Olha para o centro da cena
    glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    
    view = Matrix_Camera_View(camera_position, camera_view_vector, camera_up_vector);
}

// Função para atualizar a física do jogo
void UpdateGamePhysics(float deltaTime) {
    if (g_CurrentGameState == NAVIGATION_PHASE) {
        // Atualizar movimento do barco
        float boat_speed = g_Boat.speed * deltaTime;
        if (g_W_pressed) {
            g_Boat.position.x -= sin(g_Boat.rotation_y) * boat_speed;
            g_Boat.position.z -= cos(g_Boat.rotation_y) * boat_speed;
        }
        if (g_S_pressed) {
            g_Boat.position.x += sin(g_Boat.rotation_y) * boat_speed;
            g_Boat.position.z += cos(g_Boat.rotation_y) * boat_speed;
        }
        if (g_A_pressed) {
            g_Boat.rotation_y += 2.0f * deltaTime;
        }
        if (g_D_pressed) {
            g_Boat.rotation_y -= 2.0f * deltaTime;
        }
        g_Boat.position.y = 0.0f;
        
    } else if (g_CurrentGameState == FISHING_PHASE) {
        // Atualizar movimento do peixe na curva de Bézier
        g_Fish.bezier_t += g_Fish.speed * deltaTime;
        if (g_Fish.bezier_t > 1.0f) {
            g_Fish.bezier_t = 0.0f;
        }
        
        // Guardar posição anterior para calcular direção
        glm::vec3 old_position = g_Fish.position;
        
        g_Fish.position = CalculateBezierPoint(g_Fish.bezier_t, 
                                              g_FishBezierPoints[0], 
                                              g_FishBezierPoints[1], 
                                              g_FishBezierPoints[2], 
                                              g_FishBezierPoints[3]);
        
        // Calcular rotação baseada na direção do movimento
        glm::vec3 movement_direction = g_Fish.position - old_position;
        if (length(movement_direction) > 0.001f) {
            g_Fish.rotation_y = atan2(movement_direction.x, movement_direction.z);
        }
        
        // Atualizar o peixe com movimento aleatório
        UpdateRandomFish(deltaTime);
        
        if (g_Bait.is_launched){
            // Atualizar física da isca
            if (!g_Bait.is_in_water) {
                g_Bait.velocity.y -= 9.8f * deltaTime; // Gravidade
                g_Bait.position += g_Bait.velocity * deltaTime;
                
                // Verificar se a isca atingiu a água
                if (TestSpherePlane(g_Bait.position, g_Bait.radius, -0.3f)) {
                    g_Bait.is_in_water = true;
                    g_Bait.velocity = glm::vec3(0.0f);
                    g_Bait.position.y = -0.3f;
                    printf("Isca na água!\n");
                }
            } else {
                g_Bait.position += g_Bait.velocity * deltaTime;
            }
        }
        // Atualizar isca se estiver na água
        if (g_Bait.is_in_water) {
            float bait_speed = 2.0f;
            
            glm::vec3 control_velocity(0.0f);
            
            // Calcular direções baseadas na visão da câmera (projetada no plano do mapa)
            glm::vec3 camera_direction = glm::vec3(camera_view_vector.x, 0.0f, camera_view_vector.z);
            camera_direction = normalize(camera_direction);
            
            glm::vec4 rod_position = glm::vec4(g_Boat.position.x + camera_direction.x, 
                0.0f, g_Boat.position.z + camera_direction.z, 0.0f);

            // Calculo do vetor do caminho da vara
            glm:: vec4 bait_direction = glm::vec4(rod_position.x - g_Bait.position.x,
                0.0f, rod_position.z - g_Bait.position.z, 0.0f);
            
            if (g_W_pressed) {
                control_velocity.x += bait_direction.x * bait_speed;
                control_velocity.z += bait_direction.z * bait_speed;
            }
            
            g_Bait.velocity.x = control_velocity.x;
            g_Bait.velocity.z = control_velocity.z;
            g_Bait.velocity.y = 0.0f;

            // Tenta fisgar o peixe aleatório
            HookResult result = TryHookRandomFish(g_Bait, 0.3f);
            if (result == HOOK_FISH_CAUGHT) {
                printf("PEIXE CAPTURADO! +%d pontos\n", GetHookedFishPoints());
                g_CurrentGameState = NAVIGATION_PHASE;
                g_Bait.is_launched = false;
                g_Bait.is_in_water = false;
                // Gera novo peixe para a próxima vez
                MapArea area = GetCurrentArea(g_Boat.position);
                SpawnRandomFish(area);
            }
            else if (result == HOOK_FISH_ESCAPED) {
                // Peixe escapou, volta para navegação
                printf("Peixe escapou! Voltando para navegacao...\n");
                g_CurrentGameState = NAVIGATION_PHASE;
                g_Bait.is_launched = false;
                g_Bait.is_in_water = false;
            }
        }
    }
}




int main(int argc, char* argv[])
{


    GLFWwindow* window = InitializeWindow();

    SetupCallbacks(window);

    LoadGameResources();

    // Inicializar sistema de varas (carrega modelos 3D)
    InitializeRodSystem();

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // =====================================================================
    // Inicialização do jogo
    // =====================================================================
    InitializeGameState();
    InitializeFishingGrid();


    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Calcular deltaTime para animações baseadas em tempo
        static float last_time = 0.0f;
        float current_time = (float)glfwGetTime();
        float deltaTime = current_time - last_time;
        last_time = current_time;

        UpdateGamePhysics(deltaTime);

     
        glClearColor(0.1f, 0.3f, 0.6f, 1.0f);
        

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);


        // =====================================================================
        // Configurar câmera baseada no tipo de câmera ativo
        // =====================================================================
        glm::mat4 view;
        glm::vec4 camera_position;
        glm::mat4 projection;

        UpdateCameras(view, camera_position, projection);
  

        RenderScene(window, view, projection);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Setamos a cor do material do arquivo .mtl
    glm::vec3 material_kd = g_VirtualScene[object_name].material_kd;
    glUniform3f(g_material_kd_uniform, material_kd.x, material_kd.y, material_kd.z);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_fish_texture_id_uniform = glGetUniformLocation(g_GpuProgramID, "fish_texture_id"); // ID da textura do peixe
    g_material_kd_uniform = glGetUniformLocation(g_GpuProgramID, "material_kd"); // Cor difusa do material

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "EarthDayTexture"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "EarthNightTexture"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "BoatTexture"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "FishTexture"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "BaitTexture"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "HookTexture"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "KingfishTexture"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TroutTexture"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "PiranhaTexture"), 8);
    glUseProgram(0);
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;
    std::vector<float>  color_coefficients;  // Cores RGB do material MTL

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        // Debug: mostrar info dos materiais
        printf("  Shape '%s': %zu triangles, %zu materials loaded\n", 
               model->shapes[shape].name.c_str(), num_triangles, model->materials.size());

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            // Obtém o índice do material para esta face (triângulo)
            int material_id = -1;
            if (!model->shapes[shape].mesh.material_ids.empty()) {
                material_id = model->shapes[shape].mesh.material_ids[triangle];
            }

            // Debug: primeiro triângulo de cada shape
            if (triangle == 0) {
                printf("    -> First triangle material_id: %d\n", material_id);
            }

            // Cor difusa do material (Kd) - padrão branco se não houver material
            float r = 1.0f, g = 1.0f, b = 1.0f;
            if (material_id >= 0 && material_id < (int)model->materials.size()) {
                r = model->materials[material_id].diffuse[0];
                g = model->materials[material_id].diffuse[1];
                b = model->materials[material_id].diffuse[2];
                // Debug: mostrar cor do primeiro triângulo
                if (triangle == 0) {
                    printf("    -> Material Kd: (%.2f, %.2f, %.2f)\n", r, g, b);
                }
            }

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }

                // Adiciona a cor do material (Kd) para este vértice
                color_coefficients.push_back( r );
                color_coefficients.push_back( g );
                color_coefficients.push_back( b );
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        // Obtém a cor do material do arquivo .mtl
        int material_id = model->shapes[shape].mesh.material_ids.empty() ? -1 : model->shapes[shape].mesh.material_ids[0];
        if (material_id >= 0 && material_id < model->materials.size()) {
            theobject.material_kd = glm::vec3(
                model->materials[material_id].diffuse[0],
                model->materials[material_id].diffuse[1],
                model->materials[material_id].diffuse[2]
            );
        } else {
            // Cor padrão caso não tenha material
            theobject.material_kd = glm::vec3(0.8f, 0.8f, 0.8f);
        }

        // Se o nome já existe, adiciona um sufixo único
        std::string object_name = model->shapes[shape].name;
        int suffix = 0;
        while (g_VirtualScene.find(object_name) != g_VirtualScene.end()) {
            suffix++;
            object_name = model->shapes[shape].name + "_" + std::to_string(suffix);
        }
        theobject.name = object_name;
        g_VirtualScene[object_name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 2)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // VBO para cores do material (Kd do arquivo MTL)
    if ( !color_coefficients.empty() )
    {
        GLuint VBO_color_coefficients_id;
        glGenBuffers(1, &VBO_color_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, color_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, color_coefficients.size() * sizeof(float), color_coefficients.data());
        location = 3; // "(location = 3)" em "shader_vertex.glsl"
        number_of_dimensions = 3; // vec3 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && g_CurrentGameState == FISHING_PHASE)    
    {
        // Se estivermos na câmera de debug, mantemos o comportamento original de rotação
        if (g_CurrentCamera == DEBUG_CAMERA) {
            if (action == GLFW_PRESS) {
                glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
                g_LeftMouseButtonPressed = true;
            } else if (action == GLFW_RELEASE) {
                g_LeftMouseButtonPressed = false;
            }
        }
        // Se estivermos na câmera de jogo, usamos o sistema de carga da vara
        else {
            // PRESSIONOU: Começa a carregar ou recolhe se já estiver na água
            if (action == GLFW_PRESS) 
            {
                // Se a isca já está na água, recolhe imediatamente
                if (g_Bait.is_in_water) {
                    g_Bait.is_launched = false;
                    g_Bait.is_in_water = false;
                    g_Bait.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                    // Reposicionar isca na ponta da vara
                    g_Bait.position = GetRodTipPosition();
                    printf("Isca recolhida!\n");
                }
                // Se a isca está pronta para lançar, começa a carregar via RodSystem
                else if (!g_Bait.is_launched) {
                    StartChargingThrow();
                }
            }
            
            // SOLTOU: Realiza o lançamento com a força calculada pelo RodSystem
            else if (action == GLFW_RELEASE)
            {
                if (IsCharging()) {
                    float throw_power = ReleaseThrow(); // Pega a força calculada

                    if (!g_Bait.is_launched) {
                        g_Bait.is_launched = true;
                        g_Bait.is_in_water = false;
                        
                        // Configurar posição inicial da isca
                        g_Bait.position = glm::vec3(g_Boat.position.x, g_Boat.position.y + 0.5f, g_Boat.position.z);
                        
                        // Configurar velocidade inicial usando a força retornada
                        float launch_x = camera_view_vector.x;
                        float launch_z = camera_view_vector.z;
                        
                        g_Bait.velocity = glm::vec3(launch_x * throw_power, -1.0f, launch_z * throw_power);
                        printf("Isca lançada com força: %.2f\n", throw_power);
                    }
                }
                g_LeftMouseButtonPressed = false;
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}


void UpdateCameraAngles(double dx, double dy, float sensitivity) {
    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= sensitivity * dx;
    g_CameraPhi   += sensitivity * dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;
    
    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;
    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Câmera FPS na fase de pesca (cursor desabilitado)
    if (g_CurrentGameState == FISHING_PHASE)
    {
        // Ignorar primeiro movimento para evitar salto
        if (g_FishingFirstMouse) {
            g_FishingLastCursorX = xpos;
            g_FishingLastCursorY = ypos;
            g_FishingFirstMouse = false;
            return;
        }
        
        // Calcular deslocamento relativo
        double dx = xpos - g_FishingLastCursorX;
        double dy = ypos - g_FishingLastCursorY;
        
        g_FishingLastCursorX = xpos;
        g_FishingLastCursorY = ypos;
        
        UpdateCameraAngles(dx, dy, 0.002f);
        
        return;
    }
    
    // Câmera debug (modo livre com botão do mouse)
    if (g_LeftMouseButtonPressed)
    {
        // Calcular deslocamento desde última posição
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
        
        // Atualizar última posição
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        
        UpdateCameraAngles(dx, dy, 0.01f);
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Controlar zoom da câmera ativa
    if (g_CurrentCamera == DEBUG_CAMERA) {
        // Atualizamos a distância da câmera para a origem utilizando a
        // movimentação da "rodinha", simulando um ZOOM.
        g_CameraDistance -= 0.1f*yoffset;

        // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para o
        // onde ela está olhando, pois isto gera problemas de divisão por zero na
        // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
        // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
        // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
        const float verysmallnumber = std::numeric_limits<float>::epsilon();
        if (g_CameraDistance < verysmallnumber)
            g_CameraDistance = verysmallnumber;
    }
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Controles WASD
    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) g_W_pressed = true;
        else if (action == GLFW_RELEASE) g_W_pressed = false;
    }
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) g_A_pressed = true;
        else if (action == GLFW_RELEASE) g_A_pressed = false;
    }
    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) g_S_pressed = true;
        else if (action == GLFW_RELEASE) g_S_pressed = false;
    }
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) g_D_pressed = true;
        else if (action == GLFW_RELEASE) g_D_pressed = false;
    }
    // Alternar entre fases com Enter
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        if (g_CurrentGameState == NAVIGATION_PHASE) {
            MapArea current_area = GetCurrentArea(g_Boat.position);
            if (g_MapAreas[current_area].has_fish) {
                g_CurrentGameState = FISHING_PHASE;
                g_Bait.position = g_Boat.position;
                g_Bait.position.y = 0.5f;
                g_Bait.is_launched = false;
                g_Bait.is_in_water = false;
                
                // Gerar nova rota aleatória para o peixe
                GenerateFishRoute(current_area);
                
                // Gerar o peixe com movimento aleatório
                SpawnRandomFish(current_area);

                // Configurar câmera para começar na frente do barco
                FirstPersonCameraConfig(window);

                printf("Mudando para Fase de Pescaria na área %d\n", current_area);
            } else {
                printf("Nao há peixe nesta área! Procure uma área com peixe.\n");
            }
        } else {
          
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            g_CurrentGameState = NAVIGATION_PHASE;
            printf("Mudando para Fase de Navegação\n");
        }
    }
    
    // Alternar câmera livre com tecla C
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        if (g_CurrentCamera == GAME_CAMERA) {
            g_CurrentCamera = DEBUG_CAMERA;
            printf("Câmera Debug ativada - Use mouse para controlar\n");
        } else {
            g_CurrentCamera = GAME_CAMERA;
            printf("Câmera do jogo ativada\n");
        }
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :


// Inicializa janela GLFW e contexto OpenGL
GLFWwindow* InitializeWindow()
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Ativamos antialiasing (MSAA) com 4 amostras por pixel
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels
    GLFWwindow* window;
    window = glfwCreateWindow(1000, 800, "Carpa Diem - De Boa na Lagoa", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    return window;

}

void SetupCallbacks(GLFWwindow* window)
{
    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);
    
    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.
}

void InitializeOpenGL()
{
    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);
}


void LoadGameResources()
{
    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos as imagens para serem utilizadas como textura
    LoadTextureImage("../../data/textures/tc-earth_daymap_surface.jpg");
    LoadTextureImage("../../data/textures/tc-earth_nightmap_citylights.gif");
    LoadTextureImage("../../data/textures/boat.tga");
    LoadTextureImage("../../data/textures/fish.png");
    LoadTextureImage("../../data/textures/lure.png");
    LoadTextureImage("../../data/textures/hook.png");

    // Carrega texturas específicas dos peixes e armazena seus IDs
    for (int i = 0; i < FISH_TYPE_COUNT; i++) {
        if (g_FishTypes[i].texture_path != nullptr) {
            g_FishTypes[i].texture_id = g_NumLoadedTextures;
            LoadTextureImage(g_FishTypes[i].texture_path);
            printf("  -> Textura do %s: ID %d\n", g_FishTypes[i].name, g_FishTypes[i].texture_id);
        } else {
            g_FishTypes[i].texture_id = 3; // Usa FishTexture padrão (TextureImage3)
            printf("  -> %s usa textura padrão (ID 3)\n", g_FishTypes[i].name);
        }
    }

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel baitmodel("../../data/models/bait.obj");
    ComputeNormals(&baitmodel);
    BuildTrianglesAndAddToVirtualScene(&baitmodel);

    ObjModel boatmodel("../../data/models/boat.obj");
    ComputeNormals(&boatmodel);
    BuildTrianglesAndAddToVirtualScene(&boatmodel);

    ObjModel terrainmodel("../../data/terrain.obj", "../../data/");
    ComputeNormals(&terrainmodel);
    BuildTrianglesAndAddToVirtualScene(&terrainmodel);

    ObjModel watermodel("../../data/water.obj", "../../data/");
    ComputeNormals(&watermodel);
    BuildTrianglesAndAddToVirtualScene(&watermodel);

    ObjModel fishmodel("../../data/models/fish.obj");
    ComputeNormals(&fishmodel);
    BuildTrianglesAndAddToVirtualScene(&fishmodel);
    
    // Carregar modelos de peixes
    ObjModel anglerfish("../../data/models/fishs/Angler Fish/model.obj");
    ComputeNormals(&anglerfish);
    BuildTrianglesAndAddToVirtualScene(&anglerfish);
    
    ObjModel blowfish("../../data/models/fishs/Blowfish/Blowfish_01.obj");
    ComputeNormals(&blowfish);
    BuildTrianglesAndAddToVirtualScene(&blowfish);
    
    ObjModel kingfish("../../data/models/fishs/Kingfish/Mesh_Kingfish.obj");
    ComputeNormals(&kingfish);
    BuildTrianglesAndAddToVirtualScene(&kingfish);
    
    ObjModel trout("../../data/models/fishs/Trout/Mesh_Trout.obj");
    ComputeNormals(&trout);
    BuildTrianglesAndAddToVirtualScene(&trout);
    
    ObjModel piranha("../../data/models/fishs/Piranha/Piranha.obj");
    ComputeNormals(&piranha);
    BuildTrianglesAndAddToVirtualScene(&piranha);
    
    printf("Modelos de peixes carregados!\n");
}

void UpdateCameras(glm::mat4& view, glm::vec4& camera_position, glm::mat4& projection)
{
    if (g_CurrentCamera == DEBUG_CAMERA) {
        SetupDebugCamera(view, camera_position);
    } else if (g_CurrentGameState == NAVIGATION_PHASE) {
        SetupTopDownCamera(view, camera_position);

    } else {
        SetupFirstPersonCamera(view, camera_position);
    }

    float nearplane = -0.1f;
    float farplane  = -100.0f;

    float field_of_view = M_PI / 3.0f;
    projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

}

void RenderScene(GLFWwindow* window, const glm::mat4& view, const glm::mat4& projection)
{
    // Enviamos as matrizes "view" e "projection" para a placa de vídeo
    // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
    // efetivamente aplicadas em todos os pontos.
    glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

    // =====================================================================
    // Renderizar objetos do jogo
    // =====================================================================
    
    #define MAP             0
    #define BOAT            1
    #define FISH            2
    #define BAIT            3
    #define HOOK            4
    #define ROD             5
    #define FISHING_LINE    6
    #define TREE            7
    #define WATER           8

    // Desenhamos o terreno
    glm::mat4 model = Matrix_Translate(0.0f,-1.1f,0.0f) * Matrix_Scale(MAP_SCALE, MAP_SCALE, MAP_SCALE);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, MAP);
    DrawVirtualObject("Landscape.005");

    // Desenhamos as árvores (agora com nomes únicos Tree_1, Tree_2, etc.)
    for (int i = 1; i <= 500; i++) {
        std::string tree_name = "Tree_" + std::to_string(i);
        if (g_VirtualScene.find(tree_name) != g_VirtualScene.end()) {
            model = Matrix_Translate(0.0f,-1.1f,0.0f) * Matrix_Scale(MAP_SCALE, MAP_SCALE, MAP_SCALE);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, TREE);
            DrawVirtualObject(tree_name.c_str());
        }
    }

    // Desenhamos a água
    model = Matrix_Translate(0.0f,-1.1f,0.0f) * Matrix_Scale(MAP_SCALE, MAP_SCALE, MAP_SCALE);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WATER);
    DrawVirtualObject("Landscape_plane.002");

    // Desenhar o grid das zonas de pesca (apenas na Fase de Navegação)
    if (g_CurrentGameState == NAVIGATION_PHASE) {
        DrawFishingGrid();
    }

    // Desenhamos o barco
    model = Matrix_Translate(g_Boat.position.x, g_Boat.position.y, g_Boat.position.z) 
            * Matrix_Rotate_Y(g_Boat.rotation_y + M_PI_2) // Ajuste de orientação do modelo
            * Matrix_Scale(0.01f, 0.01f, 0.01f);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, BOAT);
    DrawVirtualObject("boat01");

    if (g_CurrentGameState == FISHING_PHASE) {
        // Renderizar vara de pesca (presa à câmera como em FPS)
        if (g_CurrentCamera == GAME_CAMERA) {
            // Posição da vara relativa à câmera (à direita e abaixo da visão)
            glm::vec3 rod_offset = g_RodDebugOffset;
            
            // Rotação da câmera
            float corrected_rotation = g_Boat.rotation_y + g_CameraTheta;
            
            // Transformação da vara: posicionar relativo à câmera
            model = Matrix_Translate(g_Boat.position.x, g_Boat.position.y + 1.0f, g_Boat.position.z)
                  * Matrix_Rotate_Y(corrected_rotation)
                  * Matrix_Translate(g_RodOffset.x, g_RodOffset.y, g_RodOffset.z)
                  * Matrix_Rotate_Y(-M_PI_2)  // Ajustar orientação da vara
                  * Matrix_Rotate_Z(-M_PI / 6.0f)  // Inclinar vara
                  * Matrix_Scale(0.08f, 0.08f, 0.08f);
            
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ROD);
            DrawVirtualObject("Fishing_Pole_Circle.001");
            
            // Desenhar linha de pesca
            FishingLineRenderInfo line_render_info;
            line_render_info.program_id = g_GpuProgramID;
            line_render_info.model_uniform = g_model_uniform;
            line_render_info.object_id_uniform = g_object_id_uniform;
            line_render_info.bbox_min_uniform = g_bbox_min_uniform;
            line_render_info.bbox_max_uniform = g_bbox_max_uniform;
            
            glm::vec3 rod_tip = GetRodTipPosition();
            
            // Se a isca não está lançada, a linha fica recolhida na ponta da vara
            glm::vec3 line_end = g_Bait.is_launched ? g_Bait.position : rod_tip;
            DrawFishingLine(rod_tip, line_end, line_render_info);
        }
        
        // Desenhamos o peixe fixo (usa textura padrão)
        model = Matrix_Translate(g_Fish.position.x, g_Fish.position.y, g_Fish.position.z) 
                * Matrix_Rotate_Y(g_Fish.rotation_y)
                * Matrix_Scale(0.1f, 0.1f, 0.1f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FISH);
        glUniform1i(g_fish_texture_id_uniform, 3); // Textura padrão
        DrawVirtualObject("fish_Cube");
        
        // Desenhamos o peixe com movimento aleatório (usa textura do tipo)
        {
            const FishTypeInfo& fishInfo = GetCurrentFishInfo();
            float scale = fishInfo.scale;
            model = Matrix_Translate(g_RandomFish.position.x, g_RandomFish.position.y, g_RandomFish.position.z) 
                    * Matrix_Rotate_Y(g_RandomFish.rotation_y)
                    * Matrix_Scale(scale, scale, scale);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, FISH);
            glUniform1i(g_fish_texture_id_uniform, fishInfo.texture_id); // Textura do tipo de peixe
            DrawVirtualObject(fishInfo.shape_name);
        }

        if (g_Bait.is_launched) {
            // Desenhamos a isca
            model = Matrix_Translate(g_Bait.position.x, g_Bait.position.y, g_Bait.position.z)
                    * Matrix_Rotate_X(-M_PI_2) // Ajuste de orientação do modelo
                    * Matrix_Scale(0.05f, 0.05f, 0.05f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BAIT);
            DrawVirtualObject("FishingLure");
            
            // Desenhamos o anzol
            model = Matrix_Translate(g_Bait.position.x, g_Bait.position.y - 0.1f, g_Bait.position.z) 
                    * Matrix_Rotate_X(-M_PI_2) // Ajuste de orientação do modelo
                    * Matrix_Scale(0.1f, 0.1f, 0.1f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, HOOK);
            DrawVirtualObject("hook");
        }
    }
    
    // Mostrar estado atual do jogo
    if (g_ShowInfoText) {
        std::string game_state = (g_CurrentGameState == NAVIGATION_PHASE) ? "FASE DE NAVEGAÇÃO" : "FASE DE PESCA";
        TextRendering_PrintString(window, "Estado: " + game_state, -1.0f, 0.9f, 1.0f);
        
        // Mostrar área atual na Fase de Navegação
        if (g_CurrentGameState == NAVIGATION_PHASE) {
            MapArea current_area = GetCurrentArea(g_Boat.position);
            bool has_fish = g_MapAreas[current_area].has_fish;
            std::string area_info = "Area: " + std::to_string((current_area % 3) + 1) + 
                                    "," + std::to_string((current_area / 3) + 1);
            if (has_fish) area_info += " (Peixe!)";
            TextRendering_PrintString(window, area_info, -1.0f, 0.8f, 1.0f);
        }
        
        TextRendering_PrintString(window, "Controles:", -1.0f, 0.7f, 1.0f);
        TextRendering_PrintString(window, "WASD - Movimento", -1.0f, 0.6f, 1.0f);
        TextRendering_PrintString(window, "Enter - Alternar Fase", -1.0f, 0.5f, 1.0f);
        TextRendering_PrintString(window, "C - Camera Livre", -1.0f, 0.4f, 1.0f);
        if (g_CurrentGameState == FISHING_PHASE) {
            TextRendering_PrintString(window, "Segure Botao Esquerdo - Carregar Lancamento", -1.0f, 0.2f, 1.0f);
            
            // Mostrar barra de força se estiver carregando
            if (IsCharging()) {
                float charge = GetCurrentChargePercentage();
                std::string charge_bar = "Forca: [";
                int bars = (int)(charge * 20.0f);
                for (int i = 0; i < 20; i++) {
                    if (i < bars) charge_bar += "|";
                    else charge_bar += ".";
                }
                charge_bar += "]";
                
                TextRendering_PrintString(window, charge_bar, -0.3f, 0.0f, 2.0f);
            }
            
            // Debug da posição da vara
            char rod_debug[100];
            snprintf(rod_debug, sizeof(rod_debug), "Rod Offset: X=%.3f Y=%.3f Z=%.3f", 
                     g_RodDebugOffset.x, g_RodDebugOffset.y, g_RodDebugOffset.z);
            TextRendering_PrintString(window, rod_debug, -1.0f, -0.2f, 1.0f);
            TextRendering_PrintString(window, "I/K - X  J/L - Y  U/O - Z  (Step: 0.01)", -1.0f, -0.3f, 1.0f);
        }
        
        // Mostrar tipo de câmera ativa
        std::string camera_type = (g_CurrentCamera == DEBUG_CAMERA) ? "CAMERA LIVRE" : "CAMERA JOGO";
        TextRendering_PrintString(window, "Camera: " + camera_type, -1.0f, 0.1f, 1.0f);
        
        if (g_CurrentCamera == DEBUG_CAMERA) {
            TextRendering_PrintString(window, "Botao Esquerdo Mouse + Arrastar - Rotacao", -1.0f, 0.0f, 1.0f);
            TextRendering_PrintString(window, "Rodinha - Zoom", -1.0f, -0.1f, 1.0f);
        }
    }

    TextRendering_ShowFramesPerSecond(window);
}