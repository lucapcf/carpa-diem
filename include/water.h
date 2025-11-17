#ifndef WATER_FRAMEBUFFER_H
#define WATER_FRAMEBUFFER_H

#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional


// Constantes dos framebuffers
#define REFLECTION_WIDTH  320
#define REFLECTION_HEIGHT 180
#define REFRACTION_WIDTH 1280
#define REFRACTION_HEIGHT 720


// Reflection Framebuffer 
struct ReflectionFBO {
    GLuint frameBuffer;
    GLuint texture;
    GLuint depthBuffer;
};

// Refraction Framebuffer
struct RefractionFBO {
    GLuint frameBuffer;
    GLuint texture;
    GLuint depthTexture;
};

struct WaterFrameBuffers {
    ReflectionFBO reflection;
    RefractionFBO refraction;
};

// Funções de gerenciamento dos framebuffers
void InitializeWaterFrameBuffers(WaterFrameBuffers* buffers);
void CleanUpWaterFrameBuffers(WaterFrameBuffers* buffers);
void BindReflectionFrameBuffer(WaterFrameBuffers* buffers);
void BindRefractionFrameBuffer(WaterFrameBuffers* buffers);
void UnbindCurrentFrameBuffer();

#endif