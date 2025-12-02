#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <glm/vec3.hpp>

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

#define ZONEMASK_W 25
#define ZONEMASK_H 25

#define WATER_SURFACE_Y -1.7f

enum ZoneType {
    ZONE_INVALID = 0,
    ZONE_VALID = 1
};

enum GameState {
    NAVIGATION_PHASE,
    FISHING_PHASE
};

enum CameraType {
    GAME_CAMERA,
    DEBUG_CAMERA
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(0.0f), max(0.0f) {}
    AABB(glm::vec3 min_val, glm::vec3 max_val) : min(min_val), max(max_val) {}
};

struct Boat {
    glm::vec3 position;
    float rotation_y;
    float speed;
    AABB bbox;
    
    Boat() : position(0.0f, 0.0f, 0.0f), rotation_y(0.0f), speed(2.0f) {}
};

struct Fish {
    glm::vec3 position;
    float bezier_t;
    float speed;
    float rotation_y;
    AABB bbox;
    
    Fish() : position(0.0f, -0.5f, 0.0f), bezier_t(0.0f), speed(0.5f), rotation_y(0.0f) {}
};

struct Bait {
    glm::vec3 position;
    glm::vec3 velocity;
    bool is_launched;
    bool is_in_water;
    float radius;
    
    Bait() : position(0.0f), velocity(0.0f), is_launched(false), is_in_water(false), radius(0.1f) {}
};

struct MapAreaInfo {
    glm::vec3 center;
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;
    bool has_fish;
    
    MapAreaInfo() : center(0.0f), min_bounds(0.0f), max_bounds(0.0f), has_fish(false) {}
    MapAreaInfo(glm::vec3 c, glm::vec3 min_b, glm::vec3 max_b, bool fish) 
        : center(c), min_bounds(min_b), max_bounds(max_b), has_fish(fish) {}
};

// Constantes do mapa
extern const float MAP_SCALE;
extern const float MAP_SIZE;
extern const float AREA_WIDTH;
extern const float AREA_HEIGHT;

#endif