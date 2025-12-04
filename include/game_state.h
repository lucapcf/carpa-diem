#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "game_types.h"

#define NUM_CUBES 3

extern GameState g_CurrentGameState;
extern CameraType g_CurrentCamera;

extern Boat g_Boat;
extern Fish g_Fish;
extern Bait g_Bait;
extern Cube g_Cubes[NUM_CUBES];

extern int g_ZoneMask[ZONEMASK_H][ZONEMASK_W];

// Pontos de controle da curva de Bézier
extern glm::vec3 g_FishBezierPoints[FISH_BEZIER_POINTS];

extern bool g_W_pressed;
extern bool g_A_pressed;
extern bool g_S_pressed;
extern bool g_D_pressed;

void InitializeGameState();

ZoneType GetZoneTypeAtPosition(glm::vec3 position);
bool IsValidBoatPosition(glm::vec3 position);

bool CheckBoatCubeCollision();

#endif