// skybox.cpp - Implementação do sistema de Skybox
// FONTE: Baseado em https://learnopengl.com/Advanced-OpenGL/Cubemaps

#include "skybox.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

// Vértices do cubo do skybox (faces voltadas para dentro)
static float skyboxVertices[] = {
    // Back face (-Z)
    -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,
    // Front face (+Z)
    -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    // Left face (-X)
    -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,
    // Right face (+X)
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
    // Bottom face (-Y)
    -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,
    // Top face (+Y)
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f
};

// Carrega e compila um shader
static GLuint LoadAndCompileShader(const char* filename, GLenum shaderType)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        fprintf(stderr, "ERROR: Cannot open shader file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    const char* source = code.c_str();
    
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "ERROR: Shader compilation failed (%s):\n%s\n", filename, log);
    }
    
    return shader;
}

// Carrega um cubemap a partir de 6 arquivos de imagem
static GLuint LoadCubemap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, channels;
    
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        stbi_set_flip_vertically_on_load(false);
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);
        
        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, 
                         width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            fprintf(stderr, "ERROR: Failed to load cubemap texture: %s\n", faces[i].c_str());
        }
    }
    
    stbi_set_flip_vertically_on_load(true); // Restaurar para outras texturas
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void InitializeSkybox(Skybox& skybox)
{
    // Criar VAO/VBO
    glGenVertexArrays(1, &skybox.VAO);
    glGenBuffers(1, &skybox.VBO);
    
    glBindVertexArray(skybox.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    
    // Criar shader program
    GLuint vs = LoadAndCompileShader("../../src/shader_skybox_vertex.glsl", GL_VERTEX_SHADER);
    GLuint fs = LoadAndCompileShader("../../src/shader_skybox_fragment.glsl", GL_FRAGMENT_SHADER);
    
    skybox.shaderProgram = glCreateProgram();
    glAttachShader(skybox.shaderProgram, vs);
    glAttachShader(skybox.shaderProgram, fs);
    glLinkProgram(skybox.shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    skybox.viewUniform = glGetUniformLocation(skybox.shaderProgram, "view");
    skybox.projectionUniform = glGetUniformLocation(skybox.shaderProgram, "projection");
    
    glUseProgram(skybox.shaderProgram);
    glUniform1i(glGetUniformLocation(skybox.shaderProgram, "skybox"), 10);
    glUseProgram(0);
    
    // Carregar texturas do cubemap
    std::vector<std::string> faces = {
        "../../data/skybox/px.png",
        "../../data/skybox/nx.png",
        "../../data/skybox/py.png",
        "../../data/skybox/ny.png",
        "../../data/skybox/pz.png",
        "../../data/skybox/nz.png"
    };
    skybox.textureID = LoadCubemap(faces);
}

void RenderSkybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    glUseProgram(skybox.shaderProgram);
    glUniformMatrix4fv(skybox.viewUniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(skybox.projectionUniform, 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(skybox.VAO);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void CleanupSkybox(Skybox& skybox)
{
    glDeleteVertexArrays(1, &skybox.VAO);
    glDeleteBuffers(1, &skybox.VBO);
    glDeleteTextures(1, &skybox.textureID);
    glDeleteProgram(skybox.shaderProgram);
}
