#version 330 core

// Shader de vértice para renderização de Skybox
// O skybox é desenhado como um cubo que sempre acompanha a câmera

layout (location = 0) in vec3 position;

out vec3 texCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Usamos a posição do vértice como coordenada de textura para o cubemap
    texCoords = position;
    
    // Removemos a translação da matriz view para que o skybox
    // sempre fique "infinitamente longe" (acompanha a câmera)
    mat4 view_no_translation = mat4(mat3(view));
    
    gl_Position = projection * view_no_translation * vec4(position, 1.0);
}
