#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

struct Skybox {
    GLuint VAO, VBO, textureID, shaderProgram;
    GLint viewUniform, projectionUniform;
};

void InitializeSkybox(Skybox& skybox);
void RenderSkybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);
void CleanupSkybox(Skybox& skybox);

#endif
