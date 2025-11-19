#include "rod_system.h"
#include <cstdio>

// Incluir a estrutura ObjModel e funções de carregamento
#include <tiny_obj_loader.h>
#include <string>
#include <vector>
#include <stdexcept>

// Estrutura para carregar modelos .obj (copiada da main.cpp)
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declarações das funções que já existem na main.cpp
extern void ComputeNormals(ObjModel* model);
extern void BuildTrianglesAndAddToVirtualScene(ObjModel* model);

void InitializeRodSystem() {
    printf("=================================================\n");
    printf("Inicializando sistema de varas de pesca...\n");
    printf("=================================================\n");
    
    // Carregar modelo 3D da vara básica
    try {
        ObjModel fishing_pole("../../data/models/fishing_pole_01.obj");
        ComputeNormals(&fishing_pole);
        BuildTrianglesAndAddToVirtualScene(&fishing_pole);
        printf("Vara básica (fishing_pole_01) carregada com sucesso!\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "ERRO ao carregar fishing_pole_01: %s\n", e.what());
    }
    
    // Carregar modelo 3D da vara avançada (fishing_rod)
    // NOTA: Este arquivo precisa ter um objeto nomeado dentro do .obj
    // Descomente quando o arquivo estiver corrigido
    /*
    try {
        ObjModel fishing_rod("../../data/models/fishing_rod.obj");
        ComputeNormals(&fishing_rod);
        BuildTrianglesAndAddToVirtualScene(&fishing_rod);
        printf("Vara avançada (fishing_rod) carregada com sucesso!\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "ERRO ao carregar fishing_rod: %s\n", e.what());
    }
    */
    
    printf("=================================================\n");
    printf("Sistema de varas inicializado!\n");
    printf("=================================================\n");
}
