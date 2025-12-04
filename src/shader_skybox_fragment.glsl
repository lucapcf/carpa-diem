#version 330 core

// Shader de fragmento para renderização de Skybox

in vec3 texCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, texCoords);
}
