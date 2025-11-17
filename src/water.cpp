#include "water.h"
#include <stdio.h>

// Funções auxiliares privadas (declaradas como static para limitar escopo ao arquivo)
 GLuint CreateFrameBuffer();
 GLuint CreateTextureAttachment(int width, int height);
 GLuint CreateDepthTextureAttachment(int width, int height);
 GLuint CreateDepthBufferAttachment(int width, int height);
 void BindFrameBuffer(GLuint frameBuffer, int width, int height);

extern int width;
extern int height;

// =====================================================================
// Funções Públicas
// =====================================================================

void InitializeWaterFrameBuffers(WaterFrameBuffers* buffers) {
    buffers->reflection.frameBuffer = CreateFrameBuffer();
    buffers->reflection.texture = CreateTextureAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    buffers->reflection.depthBuffer = CreateDepthBufferAttachment(REFLECTION_WIDTH, REFLECTION_HEIGHT);
    
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    
    buffers->refraction.frameBuffer = CreateFrameBuffer();
    buffers->refraction.texture = CreateTextureAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    buffers->refraction.depthTexture = CreateDepthTextureAttachment(REFRACTION_WIDTH, REFRACTION_HEIGHT);
    
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

}

void CleanUpWaterFrameBuffers(WaterFrameBuffers* buffers) {
    // Deletar reflexo
    glDeleteFramebuffers(1, &buffers->reflection.frameBuffer);
    glDeleteTextures(1, &buffers->reflection.texture);
    glDeleteRenderbuffers(1, &buffers->reflection.depthBuffer);
    
    // Deletar refração
    glDeleteFramebuffers(1, &buffers->refraction.frameBuffer);
    glDeleteTextures(1, &buffers->refraction.texture);
    glDeleteTextures(1, &buffers->refraction.depthTexture);
    
}

void BindReflectionFrameBuffer(WaterFrameBuffers *buffers) {
    BindFrameBuffer(buffers->reflection.frameBuffer, REFLECTION_WIDTH, REFLECTION_HEIGHT);
}

void BindRefractionFrameBuffer(WaterFrameBuffers *buffers) {
    BindFrameBuffer(buffers->refraction.frameBuffer, REFRACTION_WIDTH, REFRACTION_HEIGHT);
}

void UnbindCurrentFrameBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
}

void BindFrameBuffer(GLuint frameBuffer, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glViewport(0, 0, width, height);
}

GLuint CreateFrameBuffer() {
    GLuint frameBuffer;
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    return frameBuffer;
}

GLuint CreateTextureAttachment(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
    return texture;
}

GLuint CreateDepthTextureAttachment(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    
    return texture;
}

GLuint CreateDepthBufferAttachment(int width, int height) {
    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    
    return depthBuffer;
}