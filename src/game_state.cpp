#include "game_state.h"

// Constantes do mapa
const float MAP_SCALE = 10.0f;
const float MAP_SIZE = MAP_SCALE * 2.0f;
const float AREA_WIDTH = MAP_SIZE / 3.0f;
const float AREA_HEIGHT = MAP_SIZE / 2.0f;

// Estado global
GameState g_CurrentGameState = NAVIGATION_PHASE;
CameraType g_CurrentCamera = GAME_CAMERA;

// Objetos
Boat g_Boat;
Fish g_Fish;
Bait g_Bait;

// Áreas
MapAreaInfo g_MapAreas[NUM_AREAS];

// Curva de Bézier
glm::vec3 g_FishBezierPoints[4] = {
    glm::vec3(-3.0f, -0.5f, -2.0f),
    glm::vec3(-1.0f, -0.5f,  2.0f),
    glm::vec3( 1.0f, -0.5f, -2.0f),
    glm::vec3( 3.0f, -0.5f,  2.0f)
};

// Controles
bool g_W_pressed = false;
bool g_A_pressed = false;
bool g_S_pressed = false;
bool g_D_pressed = false;

void InitializeMapAreas() {
    float half_map = MAP_SIZE / 2.0f;
    float col_width = MAP_SIZE / 3.0f;
    float row_height = MAP_SIZE / 2.0f;
    
    g_MapAreas[AREA_1_1] = MapAreaInfo(
        glm::vec3(-half_map + col_width/2, 0.0f, half_map - row_height/2),
        glm::vec3(-half_map, -0.5f, 0.0f),
        glm::vec3(-half_map + col_width, 0.5f, half_map),
        false
    );
    
    g_MapAreas[AREA_1_2] = MapAreaInfo(
        glm::vec3(-half_map + col_width*1.5f, 0.0f, half_map - row_height/2),
        glm::vec3(-half_map + col_width, -0.5f, 0.0f),
        glm::vec3(-half_map + col_width*2, 0.5f, half_map),
        true
    );
    
    g_MapAreas[AREA_1_3] = MapAreaInfo(
        glm::vec3(-half_map + col_width*2.5f, 0.0f, half_map - row_height/2),
        glm::vec3(-half_map + col_width*2, -0.5f, 0.0f),
        glm::vec3(half_map, 0.5f, half_map),
        false
    );
    
    g_MapAreas[AREA_2_1] = MapAreaInfo(
        glm::vec3(-half_map + col_width/2, 0.0f, -half_map + row_height/2),
        glm::vec3(-half_map, -0.5f, -half_map),
        glm::vec3(-half_map + col_width, 0.5f, 0.0f),
        true
    );
    
    g_MapAreas[AREA_2_2] = MapAreaInfo(
        glm::vec3(-half_map + col_width*1.5f, 0.0f, -half_map + row_height/2),
        glm::vec3(-half_map + col_width, -0.5f, -half_map),
        glm::vec3(-half_map + col_width*2, 0.5f, 0.0f),
        false
    );
    
    g_MapAreas[AREA_2_3] = MapAreaInfo(
        glm::vec3(-half_map + col_width*2.5f, 0.0f, -half_map + row_height/2),
        glm::vec3(-half_map + col_width*2, -0.5f, -half_map),
        glm::vec3(half_map, 0.5f, 0.0f),
        true
    );
}

void InitializeGameState() {
    InitializeMapAreas();
    g_Boat.position = glm::vec3(0.0f, 0.0f, 0.0f);
    g_Boat.rotation_y = 0.0f;
}