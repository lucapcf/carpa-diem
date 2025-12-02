#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define MAP    0  // Plane como mapa
#define BOAT   1  // Boat.obj como barco
#define FISH   2  // fish como peixe
#define BAIT   3  // FishingLure (isca)
#define HOOK   4  // Hook (anzol)
#define ROD    5  // Vara de pesca
#define FISHING_LINE 6 // Linha de pesca

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D EarthDayTexture;    // TextureImage0 - Earth surface texture
uniform sampler2D EarthNightTexture;  // TextureImage1 - Earth night lights
uniform sampler2D BoatTexture;        // TextureImage2 - Boat texture
uniform sampler2D FishTexture;        // TextureImage3 - Fish texture  
uniform sampler2D BaitTexture;        // TextureImage4 - Bait texture
uniform sampler2D HookTexture;        // TextureImage5 - Hook texture

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;
in vec3 gouraud_illumination;

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    vec4 h = normalize(l + v);

    // Coordenadas de textura U e V
    vec2 uv_coords;

    // Obtemos a refletância difusa a partir da leitura da imagem de textura apropriada
    vec3 Kd0 = vec3(0.0,0.0,0.0);
    vec3 Ks0 = vec3(0.0, 0.0, 0.0); // Coeficiente de reflexão especular
    float q = 1000.0; // Expoente especular (alto = sem brilho)
    vec3 I = vec3(1.0, 1.0, 1.0); // Intensidade da luz branca


    if (object_id == MAP) {
        uv_coords = texcoords;
        Kd0 = texture(EarthDayTexture, uv_coords).rgb;
    }
    else if (object_id == BOAT) {
        uv_coords = texcoords;
        Kd0 = texture(BoatTexture, uv_coords).rgb;
        Ks0 = Kd0 * 0.5;
        q = 15.0;
    }
    else if (object_id == FISH) {
        uv_coords = texcoords;
        Kd0 = texture(FishTexture, uv_coords).rgb;
        Ks0 = Kd0 * 0.5;
        q = 20.0;
    }
    else if (object_id == BAIT) {
        uv_coords = texcoords;
        Kd0 = texture(BaitTexture, uv_coords).rgb;
    }
    else if (object_id == HOOK) {
        uv_coords = texcoords;
        Kd0 = texture(HookTexture, uv_coords).rgb;
        Ks0 = Kd0 * 0.8;
        q = 1.0;
    }
    else if (object_id == ROD) {
        // Vara de pesca: cor marrom/bege 
        uv_coords = texcoords;
        Kd0 = vec3(0.6, 0.4, 0.2);
    }
    else if (object_id == FISHING_LINE) {
        uv_coords = texcoords;
        Kd0 = vec3(1.0, 1.0, 1.0);
    }
    else {
        uv_coords = texcoords;
        // Fallback: usa EarthDayTexture
        Kd0 = texture(EarthDayTexture, uv_coords).rgb;
    }
    
    // Equação de Iluminação
    if (object_id == FISH) {
        color.rgb = Kd0 *gouraud_illumination;
    }
    else {
        float lambert = max(0,dot(n,l));

        vec3 phong_specular_term = Ks0 * I * pow(max(dot(n, h), 0.0), q);

        color.rgb = Kd0 * (lambert + 0.01) + phong_specular_term;
    }


    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

