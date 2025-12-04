#include "game_state.h"
#include "collision.h"

// Constantes do mapa
const float MAP_SCALE = 30.0f;
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
Cube g_Cubes[NUM_CUBES];

// Zone mask for valid/invalid boat positions
// 0 = invalid, 1 = valid
int g_ZoneMask[ZONEMASK_H][ZONEMASK_W] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

// Curva de Bézier
glm::vec3 g_FishBezierPoints[FISH_BEZIER_POINTS];

// Controles
bool g_W_pressed = false;
bool g_A_pressed = false;
bool g_S_pressed = false;
bool g_D_pressed = false;

ZoneType GetZoneTypeAtPosition(glm::vec3 position) {
    float half_map = MAP_SIZE / 2.0f;
    
    // Converter para intervalo [0, 1]
    float normalized_x = (position.x + half_map) / MAP_SIZE;
    float normalized_z = (position.z + half_map) / MAP_SIZE;
    
    // Converter para índices da grade
    int col = (int)(normalized_x * ZONEMASK_W);
    int row = (int)(normalized_z * ZONEMASK_H);
    
    // Fallback para intervalo válido
    if (col < 0 || col >= ZONEMASK_W || row < 0 || row >= ZONEMASK_H) {
        col = ZONEMASK_W / 2;
        row = ZONEMASK_H / 2;
    }
    
    return static_cast<ZoneType>(g_ZoneMask[row][col]);
}

bool IsValidBoatPosition(glm::vec3 position) {
    ZoneType zone = GetZoneTypeAtPosition(position);
    return zone != ZONE_INVALID;
}

void InitializeGameState() {
    g_Boat.position = glm::vec3(0.0f, 0.0f, 0.0f);
    g_Boat.rotation_y = 0.0f;
    
    g_Cubes[0] = Cube(glm::vec3(11.0f, WATER_SURFACE_Y, -4.0f), glm::vec3(1.5f, 1.5f, 1.5f));
    g_Cubes[1] = Cube(glm::vec3(-18.0f, WATER_SURFACE_Y, -17.0f), glm::vec3(0.3f, 0.3f, 0.3f));
    g_Cubes[2] = Cube(glm::vec3(21.0f, WATER_SURFACE_Y, -8.0f), glm::vec3(1.0f, 1.0f, 1.0f));
}

bool CheckBoatCubeCollision() {
    glm::vec3 boat_center = glm::vec3(g_Boat.position.x, WATER_SURFACE_Y, g_Boat.position.z);
    
    for (int i = 0; i < NUM_CUBES; i++) {
        AABB cube_aabb = g_Cubes[i].GetAABB();
        if (TestAABBSphere(cube_aabb, boat_center, g_Boat.collision_radius)) {
            return true;
        }
    }
    return false;
}
