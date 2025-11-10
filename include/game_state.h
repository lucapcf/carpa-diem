#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "game_types.h"

// Estado global do jogo
extern GameState g_CurrentGameState;
extern CameraType g_CurrentCamera;

// Objetos do jogo
extern Boat g_Boat;
extern Fish g_Fish;
extern Bait g_Bait;

// Áreas do mapa
extern MapAreaInfo g_MapAreas[NUM_AREAS];

// Pontos de controle da curva de Bézier
extern glm::vec3 g_FishBezierPoints[4];

// Controles
extern bool g_W_pressed;
extern bool g_A_pressed;
extern bool g_S_pressed;
extern bool g_D_pressed;

// Funções de inicialização
void InitializeMapAreas();
void InitializeGameState();

#endif