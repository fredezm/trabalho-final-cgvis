//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

#include "collisions.h"

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
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


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Máquina de estados da câmera.
// F cicla: CAM_DEFAULT → CAM_AERIAL → CAM_WALL → CAM_BALL → (por enquanto CAM_DEFAULT)
enum CameraState { CAM_DEFAULT, CAM_AERIAL, CAM_WALL, CAM_BALL };
CameraState g_CameraState = CAM_DEFAULT;


float g_PrevCameraTheta = 0.0f;
float g_PrevCameraPhi = 0.0f;
float g_PrevCameraDistance = 3.5f;

// Variaveis da barreira
float g_WallOffsetX = 0.0f;
// Variáveis globais para persistir a posição final da barreira
glm::vec3 g_WallCenter   = glm::vec3(0.0f);
glm::vec3 g_WallRightDir = glm::vec3(0.0f);
float     g_WallAngle    = 0.0f;

// variaveis do goleiro
float g_GoalkeeperX      = 0.0f;    // Posição X atual
float g_GoalkeeperSpeed  = 2.5f;    // Velocidade de movimento lateral
int   g_GoalkeeperDir    = 1;       // 1 = Direita, -1 = Esquerda
float g_GoalkeeperMinX   = -1.8f;   // Limite esquerdo (ajuste conforme a trave esquerda)
float g_GoalkeeperMaxX   = 1.8f;    // Limite direito (ajuste conforme a trave direita)
float g_GoalkeeperZ      = -12.7f;  // Pouco à frente da linha do gol (-13.0f)


// Posição da bola no plano do chão (X,Z). 
float g_BallPosX = 0.0f;
float g_BallPosY = 3.0f;
float g_BallPosZ = 0.0f;

// FSM da Bola
enum BallState { BALL_IDLE, BALL_POWER, BALL_BEZIER, BALL_PHYSICS };
BallState g_BallState = BALL_IDLE;

// Flag para controlar as telas de início, vitória e derrota
bool g_ShowStartScreen = true;
bool g_ShowWinScreen = false;
bool g_ShowGameOverScreen = false;

// Flag para garantir que só ocorra um chute por rodada
bool g_HasKicked = false;

// Flag para indicar que um gol foi marcado
bool g_GoalScored = false;

// Variáveis para o controle de pontuação
int g_Score = 0;
int g_TargetGoals = 3;       
int g_RemainingAttempts = 5;

// Tempo desde o início do chute
float g_TimeSinceKick = 0.0f;

// Variáveis da Barra de Força
float g_KickPower = 0.8f;       // Força atual
float g_PowerDirection = 1.0f;  // 1.0 para encher, -1.0 para esvaziar

// Velocidades da bola nos 3 eixos
float g_BallVelX = 0.0f;
float g_BallVelY = 0.0f;
float g_BallVelZ = 0.0f;

// Variáveis para "congelar" a curva no momento do chute
float g_KickTime_t = 0.0f;
glm::vec3 g_P0, g_P1, g_P2, g_P3;


// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// -----------------------------------------------------------------------
// Trajetória de Bézier cúbica para visualização do chute (g_CameraState == CAM_BALL)
// -----------------------------------------------------------------------
#define BEZIER_SEGMENTS 60          // nº de segmentos da poligonal aproximada

GLuint g_BezierVAO = 0;            // VAO da linha de trajetória
GLuint g_BezierVBO = 0;            // VBO com os pontos (atualizado todo frame)
GLuint g_BezierProgramID = 0;      // Programa de GPU minimalista (só cor sólida)

// Parâmetros da curva (editáveis pelo usuário em g_CameraState == CAM_BALL)
float g_BezierArcHeight = 0.0f;    
float g_BezierTargetY   = 0.0f;    
float g_BezierTargetX   = 0.0f;    

// Controla se a linha é desenhada por completo (true) ou pela metade (false)
bool g_ShowFullBezier = false;

// Shaders inline para a linha — sem iluminação, sem textura, apenas posição → cor
static const GLchar* const bezier_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec4 pos;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * view * pos; }\n";

static const GLchar* const bezier_fragment_src =
    "#version 330 core\n"
    "uniform vec4 lineColor;\n"
    "out vec4 color;\n"
    "void main() { color = lineColor; }\n"; // cor da linha

// Cria (na primeira chamada) ou atualiza o VAO/VBO com os pontos da curva.
// P0 = posição atual da bola, P3 = ponto final (interativo),
// P1 e P2 são pontos de controle que criam o arco do chute.
void UpdateBezierTrajectory()
{
    glm::vec3 P0(g_BallPosX, g_BallPosY, g_BallPosZ);
    glm::vec3 P3(g_BezierTargetX, g_BezierTargetY, -13.0f); // ponto final interativo

    // Pontos de controle: P1 sobe com g_BezierArcHeight; P2 usa metade desse valor
    glm::vec3 dir = P3 - P0;
    glm::vec3 P1 = P0 + dir * 0.25f + glm::vec3(0.0f, g_BezierArcHeight,        0.0f);
    glm::vec3 P2 = P0 + dir * 0.75f + glm::vec3(0.0f, g_BezierArcHeight * 0.67f, 0.0f);

    // Amostramos (BEZIER_SEGMENTS + 1) pontos ao longo da curva, t ∈ [0, 1]
    const int N = BEZIER_SEGMENTS + 1;
    glm::vec4 pts[N];
    for (int i = 0; i < N; i++)
    {
        float t  = (float)i / (float)BEZIER_SEGMENTS;
        float u  = 1.0f - t;
        // Fórmula de Bézier cúbica: B(t) = u³P0 + 3u²tP1 + 3ut²P2 + t³P3
        glm::vec3 b = u*u*u * P0
                    + 3.0f*u*u*t * P1
                    + 3.0f*u*t*t * P2
                    + t*t*t * P3;
        pts[i] = glm::vec4(b, 1.0f);
    }

    if (g_BezierVAO == 0)
    {
        glGenVertexArrays(1, &g_BezierVAO);
        glGenBuffers(1, &g_BezierVBO);

        glBindVertexArray(g_BezierVAO);
        glBindBuffer(GL_ARRAY_BUFFER, g_BezierVBO);
        // Alocamos o buffer com tamanho fixo (N pontos × vec4)
        glBufferData(GL_ARRAY_BUFFER, N * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
        // location = 0 → atributo "pos" no vertex shader da linha
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Atualiza o conteúdo do VBO com os novos pontos calculados
    glBindBuffer(GL_ARRAY_BUFFER, g_BezierVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(glm::vec4), pts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Compila um shader inline e retorna seu ID
static GLuint CompileInlineShader(GLenum type, const GLchar* src)
{
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    GLint ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        GLchar* log = new GLchar[len];
        glGetShaderInfoLog(id, len, &len, log);
        fprintf(stderr, "Bezier shader compile error:\n%s\n", log);
        delete[] log;
    }
    return id;
}

// Cria o programa de GPU minimalista para a linha de trajetória
void InitBezierShaderProgram()
{
    GLuint vs = CompileInlineShader(GL_VERTEX_SHADER,   bezier_vertex_src);
    GLuint fs = CompileInlineShader(GL_FRAGMENT_SHADER, bezier_fragment_src);
    g_BezierProgramID = glCreateProgram();
    glAttachShader(g_BezierProgramID, vs);
    glAttachShader(g_BezierProgramID, fs);
    glLinkProgram(g_BezierProgramID);
    GLint ok; glGetProgramiv(g_BezierProgramID, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0; glGetProgramiv(g_BezierProgramID, GL_INFO_LOG_LENGTH, &len);
        GLchar* log = new GLchar[len];
        glGetProgramInfoLog(g_BezierProgramID, len, &len, log);
        fprintf(stderr, "Bezier program link error:\n%s\n", log);
        delete[] log;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}
// -----------------------------------------------------------------------

// Função para resetar o estado do jogo para o início de uma nova rodada
void ResetTurn()
{
    g_BallState = BALL_IDLE;
    g_HasKicked = false;
    g_GoalScored = false;
    g_TimeSinceKick = 0.0f;
    g_BallPosX = 0.0f;
    g_BallPosY = -0.6f; 
    g_BallPosZ = 0.0f;
    g_BallVelX = 0.0f;
    g_BallVelY = 0.0f;
    g_BallVelZ = 0.0f;

    g_KickTime_t = 0.0f;
    g_BezierArcHeight = 0.0f;
    g_BezierTargetX = 0.0f;
    g_BezierTargetY = 0.0f;
    g_KickPower = 0.8f;
    g_PowerDirection = 1.0f;

    g_CameraState = CAM_AERIAL;
    g_CameraTheta = 0.0f;
    g_CameraPhi = M_PI_2;
    g_CameraDistance = 25.0f;

    g_AngleX = 0.0f; g_AngleY = 0.0f; g_AngleZ = 0.0f;

    // Sorteio de posição da barreira
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<float> dis(-2.5f, 2.5f);

    g_WallOffsetX = dis(gen);
}

void ResetGame()
{
    // Reseta o estado da bola e física
    g_BallState = BALL_IDLE;
    g_HasKicked = false;
    g_GoalScored = false;
    g_TimeSinceKick = 0.0f;
    g_ShowStartScreen = true;
    g_ShowWinScreen = false;
    g_ShowGameOverScreen = false;
    g_KickPower = 0.8f;
    g_PowerDirection = 1.0f;
    g_BallPosX = 0.0f;
    g_BallPosY = -0.6f;
    g_BallPosZ = 0.0f;
    g_BallVelX = 0.0f;
    g_BallVelY = 0.0f;
    g_BallVelZ = 0.0f;
    g_Score = 0;
    g_RemainingAttempts = 5;

    // Reseta a curva de Bézier
    g_KickTime_t = 0.0f;
    g_BezierArcHeight = 0.0f;
    g_BezierTargetX = 0.0f;
    g_BezierTargetY = 0.0f;
    g_ShowFullBezier = false;

    // Reseta a câmera para o padrão livre
    g_CameraState = CAM_DEFAULT;
    g_CameraTheta = 0.0f;
    g_CameraPhi = 0.0f;
    g_CameraDistance = 3.5f;
    
    g_AngleX = 0.0f; g_AngleY = 0.0f; g_AngleZ = 0.0f;

    // Sorteio de posição da barreira para o primeiro chute do jogo
    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<float> dis(-2.5f, 2.5f); 
    
    g_WallOffsetX = dis(gen);
}

// GLuint g_DebugVAO = 0;
// GLuint g_DebugVBO = 0;

// // Função para desenhar as linhas de uma Bounding Box na tela
// void DrawDebugAABB(glm::vec3 min, glm::vec3 max, glm::vec4 color, glm::mat4 view, glm::mat4 projection)
// {
//     if (g_DebugVAO == 0) {
//         glGenVertexArrays(1, &g_DebugVAO);
//         glGenBuffers(1, &g_DebugVBO);
//     }

//     // 8 vértices da caixa
//     glm::vec4 v[8] = {
//         glm::vec4(min.x, min.y, min.z, 1.0f), glm::vec4(max.x, min.y, min.z, 1.0f),
//         glm::vec4(max.x, max.y, min.z, 1.0f), glm::vec4(min.x, max.y, min.z, 1.0f),
//         glm::vec4(min.x, min.y, max.z, 1.0f), glm::vec4(max.x, min.y, max.z, 1.0f),
//         glm::vec4(max.x, max.y, max.z, 1.0f), glm::vec4(min.x, max.y, max.z, 1.0f)
//     };

//     // 12 linhas que formam a caixa (2 pontos por linha = 24 vértices)
//     glm::vec4 lines[24] = {
//         v[0], v[1], v[1], v[2], v[2], v[3], v[3], v[0], // Face de trás
//         v[4], v[5], v[5], v[6], v[6], v[7], v[7], v[4], // Face da frente
//         v[0], v[4], v[1], v[5], v[2], v[6], v[3], v[7]  // Conexões (profundidade)
//     };

//     glBindVertexArray(g_DebugVAO);
//     glBindBuffer(GL_ARRAY_BUFFER, g_DebugVBO);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);
//     glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
//     glEnableVertexAttribArray(0);

//     glUseProgram(g_BezierProgramID);
//     glUniform4f(glGetUniformLocation(g_BezierProgramID, "lineColor"), color.r, color.g, color.b, color.a);
//     glUniformMatrix4fv(glGetUniformLocation(g_BezierProgramID, "view"), 1, GL_FALSE, glm::value_ptr(view));
//     glUniformMatrix4fv(glGetUniformLocation(g_BezierProgramID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

//     // Desenha as linhas sobre os objetos
//     glDisable(GL_DEPTH_TEST); 
//     glLineWidth(2.0f);
//     glDrawArrays(GL_LINES, 0, 24);
//     glLineWidth(1.0f);
//     glEnable(GL_DEPTH_TEST);

//     glBindVertexArray(0);
//     glUseProgram(g_GpuProgramID); // Restaura o shader normal
// }

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "FUI RAPTADO PELO ABEL BRAGA E AGORA SOU OBRIGADO A BATER FALTAS PELO INTERNACIONAL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Inicializamos o programa de GPU minimalista para a linha de trajetória
    InitBezierShaderProgram();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");      // TextureImage0

    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1
{
    GLuint repeat_sampler;
    glGenSamplers(1, &repeat_sampler);
    glSamplerParameteri(repeat_sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(repeat_sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(repeat_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(repeat_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindSampler(1, repeat_sampler); // unit 1 = TextureImage1
}

    LoadTextureImage("../../data/Tex-Footballwhite.png"); // TextureImage2
    LoadTextureImage("../../data/Tex-Football-RM.png");   // TextureImage3

    LoadTextureImage("../../data/Pole_color.png");      // TextureImage4
    LoadTextureImage("../../data/Net.001_color.png");   // TextureImage5
    LoadTextureImage("../../data/Net.001_alpha.png");   // TextureImage6

    //LoadTextureImage("../../data/RGB_f80cd3e98591498ebe42f2fd55080acf_short01_diffuse.png"); // TextureImage7 — corpo/uniforme defesa (object_1)
    //LoadTextureImage("../../data/RGB_45e5fd0197b54093b86252868acf5166_1001_Base_Color.png"); // TextureImage8 — detalhes defesa (object_1)
    //LoadTextureImage("../../data/RGB_ec44cd734fe4425e9c02e49014d1d466_blue_eye.png"); // TextureImage9 — olhos (object_2)
    LoadTextureImage( "../../data/tex_0020_0out.jpg" );      // TextureImage7 - Uniforme/Jersey
    LoadTextureImage( "../../data/tex_0019_0out.jpg" );      // TextureImage8 - Pele/Corpo
    LoadTextureImage( "../../data/default-grey.jpg" );       // TextureImage9 - Parte extra/cabelo

    // Construímos a representação de objetos geométricos através de malhas de triângulos

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel footballmodel("../../data/FootBall.obj");
    ComputeNormals(&footballmodel);
    BuildTrianglesAndAddToVirtualScene(&footballmodel);

    ObjModel goalkeepermodel("../../data/JB8D7C8YWHEG1QC62TM93RA33.obj");
    ComputeNormals(&goalkeepermodel);
    BuildTrianglesAndAddToVirtualScene(&goalkeepermodel);
    PrintObjModelInfo(&goalkeepermodel);

    ObjModel goalmodel("../../data/Soccergoal.obj");
    ComputeNormals(&goalmodel);
    BuildTrianglesAndAddToVirtualScene(&goalmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Marcamos o tempo do último frame para a simulação física independente de FPS
    float lastTime = (float)glfwGetTime();

    // Garante que o jogo abra com as mesmas configurações de quando é reiniciado
    ResetGame();

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector;

          if (g_CameraState == CAM_BALL)
        {
            glm::vec3 ballPosition(g_BallPosX, g_BallPosY, g_BallPosZ);
            glm::vec3 goalPosition(0.0f, 0.0f, -13.0f);

            glm::vec3 dirToGoal = goalPosition - ballPosition;
            
            dirToGoal.y = 0.0f; 
            dirToGoal = glm::normalize(dirToGoal);

            // Posicionamento da Câmera
            float distanceBehind = 4.0f; // Distância que a câmera fica atrás da bola
            float heightAbove = 1.5f;    // Altura da câmera em relação à bola
            
            glm::vec3 camPos = ballPosition - (dirToGoal * distanceBehind);
            camPos.y = ballPosition.y + heightAbove; 

            camera_position_c  = glm::vec4(camPos.x, camPos.y, camPos.z, 1.0f);

            camera_lookat_l    = glm::vec4(goalPosition.x, goalPosition.y + 1.0f, goalPosition.z, 1.0f); 
            
            camera_view_vector = camera_lookat_l - camera_position_c;
            camera_up_vector   = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else if (g_CameraState == CAM_WALL)
        {
            //Posição da câmera no centro do gol (posição do goleiro). Pode ser melhorado
            camera_position_c  = glm::vec4(0.0f, 1.5f, -12.5f, 1.0f); 
            
            glm::vec4 target_ball_pos = glm::vec4(g_BallPosX, g_BallPosY, g_BallPosZ, 1.0f);
            
            camera_view_vector = target_ball_pos - camera_position_c;
            
            camera_up_vector   = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
        if (g_CameraState == CAM_AERIAL)
            camera_up_vector = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f); // Em top-down, usamos Z como up para evitar colinearidade
        else
            camera_up_vector = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" padrão apontando para o "céu" (eixo Y global)
        }

        
        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROmakORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));


        #define PLANE           2        
        #define FOOTBALL        3
        #define GOAL_POLE_IRON  4
        #define GOAL_NET        5
        #define GOAL_POLE       6
        #define GOALKEEPER      7
        #define GOALKEEPER_HAIR 8
        #define GOALKEEPER_EXTRA 9

        // Desenhamos o plano do chão
        model = Matrix_Translate(0.0f,-1.1f,0.0f)
              * Matrix_Scale(15.0f, 1.0f, 15.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");
        
        // Colisão entre bola e plano do chão, atualizando a posição e velocidade da bola.
        // FSM da bola
        // Cálculo de tempo.
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        // Conta o tempo após o chute.
        if (g_HasKicked) {
            g_TimeSinceKick += deltaTime;
        }
        
        // Movimentação do goleiro
        g_GoalkeeperX += g_GoalkeeperDir * g_GoalkeeperSpeed * deltaTime;

        // Inverte a direção ao tocar nos limites das traves
        if (g_GoalkeeperX >= g_GoalkeeperMaxX) {
            g_GoalkeeperX = g_GoalkeeperMaxX;
            g_GoalkeeperDir = -1;
        }
        else if (g_GoalkeeperX <= g_GoalkeeperMinX) {
            g_GoalkeeperX = g_GoalkeeperMinX;
            g_GoalkeeperDir = 1;
        }

        // =================================================================
        // LÓGICA DA BARREIRA (Posição e Orientação)
        // Dentro do loop principal (antes da física e da renderização):
        if (!g_HasKicked)
        {
            glm::vec3 goalPos(0.0f, 0.0f, -13.0f);
            glm::vec3 ballPosPlane(g_BallPosX, 0.0f, g_BallPosZ);
            
            // Direção da bola para o gol
            glm::vec3 dirToGoalWall = glm::normalize(goalPos - ballPosPlane);
            
            // Vetor perpendicular para o deslocamento lateral
            g_WallRightDir = glm::vec3(-dirToGoalWall.z, 0.0f, dirToGoalWall.x);

            float wallDistance = 4.0f; // Distância fixa da barreira até a bola
            
            // Calcula e armazena a posição centralizada da barreira com o offset do jogador
            g_WallCenter = ballPosPlane + (dirToGoalWall * wallDistance) + (g_WallRightDir * g_WallOffsetX);
            g_WallCenter.y = 0.15f; // Alinhado ao chão

            // Ângulo de rotação para os defensores encararem a bola
            g_WallAngle = atan2(dirToGoalWall.x, dirToGoalWall.z);
        }
        // =================================================================

        // Comportamento baseado no estado
        if (g_BallState == BALL_POWER)
        {
            // A barra oscila no tempo
            float baseSpeed = 1.0f;
            float dynamicSpeed = baseSpeed + (g_KickPower * g_KickPower * 0.8f);
            g_KickPower += g_PowerDirection * dynamicSpeed * deltaTime;

            // Limites da força (0.8 = força mínima, 1.7 = força máxima)
            if (g_KickPower >= 1.7f) {
                g_KickPower = 1.7f;
                g_PowerDirection = -1.0f; // Começa a esvaziar
            } 
            else if (g_KickPower <= 0.8f) {
                g_KickPower = 0.8f;
                g_PowerDirection = 1.0f; // Começa a encher
            }
        }
        else if (g_BallState == BALL_BEZIER)
        {
            float kickSpeed = g_KickPower; 
            g_KickTime_t += deltaTime * kickSpeed;

            if (g_KickTime_t >= 1.0f)
            {
                g_KickTime_t = 1.0f;
                g_BallState = BALL_PHYSICS;

                // Calcula a velocidade de saída da curva (tangente em t=1)
                glm::vec3 exitVelocity = 3.0f * (g_P3 - g_P2) * kickSpeed;
                g_BallVelX = exitVelocity.x;
                g_BallVelY = exitVelocity.y;
                g_BallVelZ = exitVelocity.z;
            }
                else
            {
                float t = g_KickTime_t;
                float u = 1.0f - t;
            
                glm::vec3 pos =
                    u*u*u*g_P0 +
                    3.0f*u*u*t*g_P1 +
                    3.0f*u*t*t*g_P2 +
                    t*t*t*g_P3;
            
                float ballRadius = 0.5f;
            
                // ==========================
                // AABBs da barreira
                // ==========================
            
                float wallDefHalfWidth = 0.5706f + 0.05f;
                float wallDefHalfDepth = 0.2372f + 0.05f;
                float wallDefHalfDiag =
                    sqrtf(wallDefHalfWidth * wallDefHalfWidth +
                          wallDefHalfDepth * wallDefHalfDepth);
            
                float wallDefBaseY  = -1.1f;
                float wallDefHeight = 1.74f;
                float wallDefSep    = 0.6f;
            
                glm::vec3 wallDef1Pos =
                    g_WallCenter - (g_WallRightDir * wallDefSep);
            
                glm::vec3 wallDef2Pos =
                    g_WallCenter + (g_WallRightDir * wallDefSep);
            
                glm::vec3 wallDefMin[] =
                {
                    glm::vec3(
                        wallDef1Pos.x - wallDefHalfDiag,
                        wallDefBaseY,
                        wallDef1Pos.z - wallDefHalfDiag),
            
                    glm::vec3(
                        wallDef2Pos.x - wallDefHalfDiag,
                        wallDefBaseY,
                        wallDef2Pos.z - wallDefHalfDiag)
                };
            
                glm::vec3 wallDefMax[] =
                {
                    glm::vec3(
                        wallDef1Pos.x + wallDefHalfDiag,
                        wallDefBaseY + wallDefHeight,
                        wallDef1Pos.z + wallDefHalfDiag),
            
                    glm::vec3(
                        wallDef2Pos.x + wallDefHalfDiag,
                        wallDefBaseY + wallDefHeight,
                        wallDef2Pos.z + wallDefHalfDiag)
                };
            
                bool hitWall = false;
            
                for (int i = 0; i < 2; i++)
                {
                    if (TestIntersectionSphereAABB(
                            pos,
                            ballRadius,
                            wallDefMin[i],
                            wallDefMax[i]))
                    {
                        hitWall = true;
                        break;
                    }
                }
            
                // ==========================
                // Colisão durante a Bézier
                // ==========================
            
                if (hitWall)
                {
                    g_BallPosX = pos.x;
                    g_BallPosY = pos.y;
                    g_BallPosZ = pos.z;
            
                    // Tangente da Bézier
                    glm::vec3 tangent =
                        3.0f*u*u*(g_P1 - g_P0)
                      + 6.0f*u*t*(g_P2 - g_P1)
                      + 3.0f*t*t*(g_P3 - g_P2);
            
                    tangent = glm::normalize(tangent);
            
                    float launchSpeed = 12.0f * g_KickPower;
            
                    g_BallVelX = tangent.x * launchSpeed;
                    g_BallVelY = tangent.y * launchSpeed;
                    g_BallVelZ = tangent.z * launchSpeed;
            
                    g_BallState = BALL_PHYSICS;
                }
                else
                {
                    g_BallPosX = pos.x;
                    g_BallPosY = pos.y;
                    g_BallPosZ = pos.z;
                }
    
        }   }
        else if (g_BallState == BALL_PHYSICS)
        {
            // Aplica gravidade e movimento
            g_BallVelY += -9.81f * deltaTime;   
            g_BallPosX += g_BallVelX * deltaTime;
            g_BallPosY += g_BallVelY * deltaTime; 
            g_BallPosZ += g_BallVelZ * deltaTime;
            
            // Colisão com o Chão
            glm::vec3 ballCenter(g_BallPosX, g_BallPosY, g_BallPosZ);
            float ballRadius = 0.5f;
            float groundHeight = -1.1f;

            if (TestIntersectionSpherePlane(ballCenter, ballRadius, groundHeight)) 
            {
                g_BallPosY = groundHeight + ballRadius; 
                g_BallVelY = -g_BallVelY * 0.7f; // Quique da bola
                
                // Atrito horizontal
                g_BallVelX *= 0.98f; 
                g_BallVelZ *= 0.98f;

                if (g_BallVelY < 0.1f && g_BallVelY > -0.1f) g_BallVelY = 0.0f; 
                
                // Parada total
                if (g_BallVelY == 0.0f && std::abs(g_BallVelX) < 0.1f && std::abs(g_BallVelZ) < 0.1f) {
                    g_BallState = BALL_IDLE;
                    g_BallVelX = g_BallVelZ = 0.0f;
                }
            }

            // Colisão com traves e travessão (Hitboxes Manuais)
            float goalZ = -13.0f;        // Posição Z exata do gol
            float postX = 4.0f;          // Distância do centro até as traves (ajuste para alinhar com o visual)
            float postThickness = 0.2f;  // Espessura da trave
            float crossbarHeight = 1.8f; // Altura do travessão (ajuste se bater no ar)
            float gkWidth  = 1.2f;  // Largura  da Hitbox do goleiro
            float gkHeight = 2.0f;  // Altura hitbox goleiro
            float gkDepth  = 0.8f;  // Espessura hitbox goleiro

            glm::vec3 leftPostMin(-postX - postThickness, -1.1f, goalZ - postThickness);
            glm::vec3 leftPostMax(-postX + postThickness, crossbarHeight, goalZ + postThickness);

            glm::vec3 rightPostMin(postX - postThickness, -1.1f, goalZ - postThickness);
            glm::vec3 rightPostMax(postX + postThickness, crossbarHeight, goalZ + postThickness);

            glm::vec3 crossbarMin(-postX, crossbarHeight - postThickness, goalZ - postThickness);
            glm::vec3 crossbarMax(postX, crossbarHeight + postThickness, goalZ + postThickness);

            glm::vec3 hitboxesMin[] = {leftPostMin, rightPostMin, crossbarMin};
            glm::vec3 hitboxesMax[] = {leftPostMax, rightPostMax, crossbarMax};


            // Testa colisões individualmente
            for (int i = 0; i < 3; i++)
            {
                glm::vec3 aabbMin = hitboxesMin[i];
                glm::vec3 aabbMax = hitboxesMax[i];

                if (TestIntersectionSphereAABB(ballCenter, ballRadius, aabbMin, aabbMax)) 
                {
                    // Encontra o ponto da superfície da trave que está mais próximo da bola
                    float px = std::max(aabbMin.x, std::min(ballCenter.x, aabbMax.x));
                    float py = std::max(aabbMin.y, std::min(ballCenter.y, aabbMax.y));
                    float pz = std::max(aabbMin.z, std::min(ballCenter.z, aabbMax.z));
                    
                    glm::vec3 closestPoint(px, py, pz);
                    glm::vec3 distVec = ballCenter - closestPoint;
                    float dist = glm::length(distVec);
                    
                    glm::vec3 normal(0.0f, 1.0f, 0.0f);
                    
                    // Empurra a bola fisicamente para fora até que a borda (raio) cole na trave, e calcula a normal real da colisão
                    if (dist > 0.001f) {
                        normal = distVec / dist;
                        float penetration = ballRadius - dist; 
                        
                        g_BallPosX += normal.x * penetration;
                        g_BallPosY += normal.y * penetration;
                        g_BallPosZ += normal.z * penetration;
                    }
                    // Caso extremo de alta velocidade onde a bola penetra completamente a trave, aproximamos a normal pela direção oposta da velocidade
                    else 
                    {
                        glm::vec3 velDir = glm::normalize(glm::vec3(g_BallVelX, 0.0f, g_BallVelZ));
                        g_BallPosX -= velDir.x * ballRadius;
                        g_BallPosZ -= velDir.z * ballRadius;
                        normal = -velDir;
                    }

                    // Reflexão espelhada
                    glm::vec3 currentVel(g_BallVelX, g_BallVelY, g_BallVelZ);
                    
                    // Calcula o rebatimento baseado no ângulo exato da batida
                    glm::vec3 reflectedVel = glm::reflect(currentVel, normal);
                    
                    float restitution = 0.75f; // Perde 25% da energia ao bater na trave
                    
                    g_BallVelX = reflectedVel.x * restitution;
                    g_BallVelY = reflectedVel.y * restitution;
                    g_BallVelZ = reflectedVel.z * restitution;

                    break;
                }
            } 
            // Colisão com o goleiro (Hitbox Manual)
            // Define a caixa de colisão baseada na posição dinâmica g_GoalkeeperX
            glm::vec3 gkMin(g_GoalkeeperX - gkWidth/2,  -1.1f,            g_GoalkeeperZ - gkDepth/2);
            glm::vec3 gkMax(g_GoalkeeperX + gkWidth/2,  -1.1f + gkHeight, g_GoalkeeperZ + gkDepth/2);

            if (TestIntersectionSphereAABB(ballCenter, ballRadius, gkMin, gkMax)) 
            {
                // Encontra o ponto mais próximo na caixa para calcular a normal de colisão
                float px = std::max(gkMin.x, std::min(ballCenter.x, gkMax.x));
                float py = std::max(gkMin.y, std::min(ballCenter.y, gkMax.y));
                float pz = std::max(gkMin.z, std::min(ballCenter.z, gkMax.z));
                
                glm::vec3 closestPoint(px, py, pz);
                glm::vec3 distVec = ballCenter - closestPoint;
                float dist = glm::length(distVec);
                
                glm::vec3 normal(0.0f, 0.0f, 1.0f); // Direção padrão de rebatimento (para frente do gol)
                
                if (dist > 0.001f) {
                    normal = distVec / dist;
                    float penetration = ballRadius - dist; 
                    // Afasta a bola para não ficar presa dentro do goleiro
                    g_BallPosX += normal.x * penetration;
                    g_BallPosY += normal.y * penetration;
                    g_BallPosZ += normal.z * penetration;
                }
                else{
                    glm::vec3 velDir = glm::normalize(glm::vec3(g_BallVelX, 0.0f, g_BallVelZ));
                    g_BallPosX -= velDir.x * ballRadius;
                    g_BallPosZ -= velDir.z * ballRadius;
                    normal = -velDir;
                }
                ballCenter = glm::vec3(g_BallPosX, g_BallPosY, g_BallPosZ);

                // Aplica a reflexão do vetor velocidade
                glm::vec3 currentVel(g_BallVelX, g_BallVelY, g_BallVelZ);
                glm::vec3 reflectedVel = glm::reflect(currentVel, normal);
                
                float gkRestitution = 0.5f; // Elasticidade do impacto com o goleiro
                
                g_BallVelX = reflectedVel.x * gkRestitution;
                g_BallVelY = reflectedVel.y * gkRestitution;
                g_BallVelZ = reflectedVel.z * gkRestitution;
            }            

            // Colisão com a barreira de defensores 

            float wallDefHalfWidth = 0.62f; 
            float wallDefHalfDepth = 0.28f;
            float wallDefHalfDiag  = sqrtf(wallDefHalfWidth * wallDefHalfWidth + wallDefHalfDepth * wallDefHalfDepth);
            float wallDefBaseY     = -1.1f;          // pés alinhados ao chão, igual ao plano
            float wallDefHeight    = 1.74f;          // altura do modelo (eixo Y local)
            float wallDefSep       = 0.6f;           // mesma separação usada ao desenhar (distEntreDefensores)

            glm::vec3 wallDef1Pos = g_WallCenter - (g_WallRightDir * wallDefSep);
            glm::vec3 wallDef2Pos = g_WallCenter + (g_WallRightDir * wallDefSep);

            glm::vec3 wallDefMin[] = {
                glm::vec3(wallDef1Pos.x - wallDefHalfDiag, wallDefBaseY, wallDef1Pos.z - wallDefHalfDiag),
                glm::vec3(wallDef2Pos.x - wallDefHalfDiag, wallDefBaseY, wallDef2Pos.z - wallDefHalfDiag)
            };
            glm::vec3 wallDefMax[] = {
                glm::vec3(wallDef1Pos.x + wallDefHalfDiag, wallDefBaseY + wallDefHeight, wallDef1Pos.z + wallDefHalfDiag),
                glm::vec3(wallDef2Pos.x + wallDefHalfDiag, wallDefBaseY + wallDefHeight, wallDef2Pos.z + wallDefHalfDiag)
            };

            for (int i = 0; i < 2; i++)
            {
                glm::vec3 aabbMin = wallDefMin[i];
                glm::vec3 aabbMax = wallDefMax[i];

                if (TestIntersectionSphereAABB(ballCenter, ballRadius, aabbMin, aabbMax))
                {
                    // Encontra o ponto da superfície do defensor que está mais próximo da bola
                    float px = std::max(aabbMin.x, std::min(ballCenter.x, aabbMax.x));
                    float py = std::max(aabbMin.y, std::min(ballCenter.y, aabbMax.y));
                    float pz = std::max(aabbMin.z, std::min(ballCenter.z, aabbMax.z));

                    glm::vec3 closestPoint(px, py, pz);
                    glm::vec3 distVec = ballCenter - closestPoint;
                    float dist = glm::length(distVec);

                    glm::vec3 normal(0.0f, 1.0f, 0.0f);

                    // Empurra a bola fisicamente para fora até a borda (raio) encostar no defensor, calculando a normal real
                    if (dist > 0.001f) {
                        normal = distVec / dist;
                        float penetration = ballRadius - dist;

                        g_BallPosX += normal.x * penetration;
                        g_BallPosY += normal.y * penetration;
                        g_BallPosZ += normal.z * penetration;
                    }
                    // Caso extremo de alta velocidade onde a bola penetra completamente o defensor
                    else
                    {
                        glm::vec3 velDir = glm::normalize(glm::vec3(g_BallVelX, 0.0f, g_BallVelZ));
                        g_BallPosX -= velDir.x * ballRadius;
                        g_BallPosZ -= velDir.z * ballRadius;
                        normal = -velDir;
                    }

                    // Atualiza ballCenter para refletir o empurrão acima de imediato
                    ballCenter = glm::vec3(g_BallPosX, g_BallPosY, g_BallPosZ);

                    // Reflexão espelhada
                    glm::vec3 currentVel(g_BallVelX, g_BallVelY, g_BallVelZ);
                    glm::vec3 reflectedVel = glm::reflect(currentVel, normal);

                    float restitution = 0.6f; // Defensor amortece mais o impacto do que a trave (menos rígido)

                    g_BallVelX = reflectedVel.x * restitution;
                    g_BallVelY = reflectedVel.y * restitution;
                    g_BallVelZ = reflectedVel.z * restitution;

                    break;
                }
            }

            glm::vec3 scoreMin(-3.7f, -1.0f, -14.3f); // Limite inferior (dentro do gol)
            glm::vec3 scoreMax( 3.7f,  1.6f, -13.6f); // Limite superior (dentro do gol)

            // Testamos se a bola está cruzando a caixa de gol E se ainda não marcou
            if (!g_GoalScored && TestIntersectionSphereAABB(ballCenter, ballRadius, scoreMin, scoreMax)) 
            {
                printf("          GOL!          \n");
                g_GoalScored = true;
                g_Score++;
            }

            // Colisão com rede
            float netDepth = 1.4f; 
            float netFrontZ = goalZ - postThickness;
            float netBackZ = goalZ - netDepth; 
            float netThickness = 0.1f; 

            // 1. Rede Esquerda
            glm::vec3 netLeftMin(-postX - postThickness - netThickness, -1.1f, netBackZ);
            glm::vec3 netLeftMax(-postX - postThickness, crossbarHeight, netFrontZ);

            // 2. Rede Direita
            glm::vec3 netRightMin(postX + postThickness, -1.1f, netBackZ);
            glm::vec3 netRightMax(postX + postThickness + netThickness, crossbarHeight, netFrontZ);

            // 3. Rede Superior (Teto)
            glm::vec3 netTopMin(-postX - postThickness, crossbarHeight, netBackZ);
            glm::vec3 netTopMax(postX + postThickness, crossbarHeight + netThickness, netFrontZ);

            // 4. Rede Fundo (Traseira)
            glm::vec3 netBackMin(-postX - postThickness, -1.1f, netBackZ - netThickness);
            glm::vec3 netBackMax(postX + postThickness, crossbarHeight, netBackZ);

            glm::vec3 netHitboxesMin[] = {netLeftMin, netRightMin, netTopMin, netBackMin};
            glm::vec3 netHitboxesMax[] = {netLeftMax, netRightMax, netTopMax, netBackMax};

            // Testa as colisões com a rede
            for (int i = 0; i < 4; i++)
            {
                glm::vec3 aabbMin = netHitboxesMin[i];
                glm::vec3 aabbMax = netHitboxesMax[i];

                if (TestIntersectionSphereAABB(ballCenter, ballRadius, aabbMin, aabbMax)) 
                {
                    
                    float px = std::max(aabbMin.x, std::min(ballCenter.x, aabbMax.x));
                    float py = std::max(aabbMin.y, std::min(ballCenter.y, aabbMax.y));
                    float pz = std::max(aabbMin.z, std::min(ballCenter.z, aabbMax.z));
                    
                    glm::vec3 closestPoint(px, py, pz);
                    glm::vec3 distVec = ballCenter - closestPoint;
                    float dist = glm::length(distVec);
                    
                    glm::vec3 normal(0.0f, 1.0f, 0.0f);
                    
                    if (dist > 0.001f) {
                        normal = distVec / dist;
                        float penetration = ballRadius - dist; 
                        g_BallPosX += normal.x * penetration;
                        g_BallPosY += normal.y * penetration;
                        g_BallPosZ += normal.z * penetration;
                    } else {
                        glm::vec3 velDir = glm::normalize(glm::vec3(g_BallVelX, g_BallVelY, g_BallVelZ));
                        g_BallPosX -= velDir.x * ballRadius;
                        g_BallPosY -= velDir.y * ballRadius;
                        g_BallPosZ -= velDir.z * ballRadius;
                        normal = -velDir;
                    }

                    // Amortecimento da rede
                    glm::vec3 currentVel(g_BallVelX, g_BallVelY, g_BallVelZ);
                    glm::vec3 reflectedVel = glm::reflect(currentVel, normal);

                    // Verifica se o centro da bola está fora do volume interno do gol
                    bool isOutside = (ballCenter.x < -postX || 
                                      ballCenter.x > postX || 
                                      ballCenter.y > crossbarHeight || 
                                      ballCenter.z < netBackZ);
                    
                    // Se bateu por fora, rebate.
                    // Se bateu por dentro, amortece.
                    float netRestitution = isOutside ? 0.6f : 0.15f;
                    
                    g_BallVelX = reflectedVel.x * netRestitution;
                    g_BallVelY = reflectedVel.y * netRestitution;
                    g_BallVelZ = reflectedVel.z * netRestitution;

                    break; // Sai do loop após colidir com uma das placas da rede
                }
            }    
        }
        // Desenho bola de futebol
        model = Matrix_Translate(g_BallPosX, g_BallPosY, g_BallPosZ)
              * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FOOTBALL);
        DrawVirtualObject("FootBall");

        // =================================================================
        // DESENHAR BARREIRA na camera do goleiro ou da bola
        // =================================================================
        if (g_CameraState == CAM_WALL || g_CameraState == CAM_BALL)
        {
            float distEntreDefensores = 0.6f; // Distância do centro para cada boneco

            // Defensor 1 (Esquerda)
            glm::vec3 def1Pos = g_WallCenter - (g_WallRightDir * distEntreDefensores);
            model = Matrix_Translate(def1Pos.x, def1Pos.y, def1Pos.z)
                * Matrix_Rotate_Y(g_WallAngle + (-M_PI/2.0f))
                * Matrix_Scale(0.85f, 0.85f, 0.85f); 
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, GOALKEEPER);
            DrawVirtualObject("Object_tex_0020_0out.jpg");   // corpo
            glUniform1i(g_object_id_uniform, GOALKEEPER_HAIR);
            DrawVirtualObject("Object_tex_0019_0out.jpg");   // rosto

            // Defensor 2 (Direita)
            glm::vec3 def2Pos = g_WallCenter + (g_WallRightDir * distEntreDefensores);
            model = Matrix_Translate(def2Pos.x, def2Pos.y, def2Pos.z)
                * Matrix_Rotate_Y(g_WallAngle + (-M_PI/2.0f))
                * Matrix_Scale(0.85f, 0.85f, 0.85f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, GOALKEEPER);
            DrawVirtualObject("Object_tex_0020_0out.jpg");   // corpo
            glUniform1i(g_object_id_uniform, GOALKEEPER_HAIR);
            DrawVirtualObject("Object_tex_0019_0out.jpg");   // rosto

            // =================================================================
            // DESENHAR GOLEIRO NA LINHA DO GOL
            // =================================================================
            model = Matrix_Translate(g_GoalkeeperX, 0.10f, g_GoalkeeperZ)
                * Matrix_Rotate_Y(M_PI/2.0f)
                * Matrix_Scale(0.85f, 0.85f, 0.85f); 

            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, GOALKEEPER);
            DrawVirtualObject("Object_tex_0020_0out.jpg");   // corpo
            glUniform1i(g_object_id_uniform, GOALKEEPER_HAIR);
            DrawVirtualObject("Object_tex_0019_0out.jpg");   // rosto

        }

        model = Matrix_Translate(0.0f, -1.1f, -13.0f)
            * Matrix_Scale(3.0f, 1.5f, 1.5f);

        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, GOAL_POLE_IRON);
        DrawVirtualObject("pole.001_BezierCurve.001_IronPole");

        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, GOAL_POLE);
        DrawVirtualObject("Pole_Cube.001_Pole");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, GOAL_NET);
        DrawVirtualObject("Net.001_Plane.003_Net");
        glDisable(GL_BLEND);

        // // Renderiza hitboxes das traves e travessao para debug (copiar e colar valores para ajustar)
        // float dbg_goalZ = -13.0f;        
        // float dbg_postX = 4.0f;          
        // float dbg_postThickness = 0.2f;  
        // float dbg_crossbarHeight = 1.8f; 

        // glm::vec3 dbg_leftMin(-dbg_postX - dbg_postThickness, -1.1f, dbg_goalZ - dbg_postThickness);
        // glm::vec3 dbg_leftMax(-dbg_postX + dbg_postThickness, dbg_crossbarHeight, dbg_goalZ + dbg_postThickness);

        // glm::vec3 dbg_rightMin(dbg_postX - dbg_postThickness, -1.1f, dbg_goalZ - dbg_postThickness);
        // glm::vec3 dbg_rightMax(dbg_postX + dbg_postThickness, dbg_crossbarHeight, dbg_goalZ + dbg_postThickness);

        // glm::vec3 dbg_crossbarMin(-dbg_postX, dbg_crossbarHeight - dbg_postThickness, dbg_goalZ - dbg_postThickness);
        // glm::vec3 dbg_crossbarMax(dbg_postX, dbg_crossbarHeight + dbg_postThickness, dbg_goalZ + dbg_postThickness);

        // // Renderiza as caixas amarelas de colisão
        // DrawDebugAABB(dbg_leftMin, dbg_leftMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // DrawDebugAABB(dbg_rightMin, dbg_rightMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // DrawDebugAABB(dbg_crossbarMin, dbg_crossbarMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);

        // // Renderiza hitboxes da barreira de defensores para debug (mesmos valores usados na colisão acima)
        // {
        //     float dbg_wallHalfWidth = 0.5706f + 0.05f;
        //     float dbg_wallHalfDepth = 0.2372f + 0.05f;
        //     float dbg_wallHalfDiag  = sqrtf(dbg_wallHalfWidth * dbg_wallHalfWidth + dbg_wallHalfDepth * dbg_wallHalfDepth);
        //     float dbg_wallBaseY     = -1.1f;
        //     float dbg_wallHeight    = 1.74f;
        //     float dbg_wallSep       = 0.6f;

        //     glm::vec3 dbg_wallDef1Pos = g_WallCenter - (g_WallRightDir * dbg_wallSep);
        //     glm::vec3 dbg_wallDef2Pos = g_WallCenter + (g_WallRightDir * dbg_wallSep);

        //     glm::vec3 dbg_wallDef1Min(dbg_wallDef1Pos.x - dbg_wallHalfDiag, dbg_wallBaseY, dbg_wallDef1Pos.z - dbg_wallHalfDiag);
        //     glm::vec3 dbg_wallDef1Max(dbg_wallDef1Pos.x + dbg_wallHalfDiag, dbg_wallBaseY + dbg_wallHeight, dbg_wallDef1Pos.z + dbg_wallHalfDiag);

        //     glm::vec3 dbg_wallDef2Min(dbg_wallDef2Pos.x - dbg_wallHalfDiag, dbg_wallBaseY, dbg_wallDef2Pos.z - dbg_wallHalfDiag);
        //     glm::vec3 dbg_wallDef2Max(dbg_wallDef2Pos.x + dbg_wallHalfDiag, dbg_wallBaseY + dbg_wallHeight, dbg_wallDef2Pos.z + dbg_wallHalfDiag);

        //     DrawDebugAABB(dbg_wallDef1Min, dbg_wallDef1Max, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        //     DrawDebugAABB(dbg_wallDef2Min, dbg_wallDef2Max, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // }

        // // Renderiza hitboxes das redes para debug (copiar e colar valores para ajustar)
        // // Variáveis para você ajustar o tamanho e posição da rede
        // float dbg_netDepth = 1.4f; // O quão profunda a rede vai para trás
        // float dbg_netFrontZ = dbg_goalZ - dbg_postThickness; // Define que a rede começa exatamente atrás da trave
        // float dbg_netBackZ = dbg_goalZ - dbg_netDepth; // Calcula o Z final da rede
        // float dbg_netThickness = 0.1f; // Placa fininha para a rede

        // // 1. Rede Esquerda (Placa lateral)
        // glm::vec3 dbg_netLeftMin(-dbg_postX - dbg_postThickness - dbg_netThickness, -1.1f, dbg_netBackZ);
        // glm::vec3 dbg_netLeftMax(-dbg_postX - dbg_postThickness, dbg_crossbarHeight, dbg_netFrontZ);

        // // 2. Rede Direita (Placa lateral)
        // glm::vec3 dbg_netRightMin(dbg_postX + dbg_postThickness, -1.1f, dbg_netBackZ);
        // glm::vec3 dbg_netRightMax(dbg_postX + dbg_postThickness + dbg_netThickness, dbg_crossbarHeight, dbg_netFrontZ);

        // // 3. Rede Superior (Teto)
        // glm::vec3 dbg_netTopMin(-dbg_postX - dbg_postThickness, dbg_crossbarHeight, dbg_netBackZ);
        // glm::vec3 dbg_netTopMax(dbg_postX + dbg_postThickness, dbg_crossbarHeight + dbg_netThickness, dbg_netFrontZ);

        // // 4. Rede Fundo (Traseira)
        // glm::vec3 dbg_netBackMin(-dbg_postX - dbg_postThickness, -1.1f, dbg_netBackZ - dbg_netThickness);
        // glm::vec3 dbg_netBackMax(dbg_postX + dbg_postThickness, dbg_crossbarHeight, dbg_netBackZ);

        // // Desenha as caixas da rede
        // DrawDebugAABB(dbg_netLeftMin, dbg_netLeftMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // DrawDebugAABB(dbg_netRightMin, dbg_netRightMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // DrawDebugAABB(dbg_netTopMin, dbg_netTopMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);
        // DrawDebugAABB(dbg_netBackMin, dbg_netBackMax, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), view, projection);

        // // Renderiza a caixa verde do gatilho de Gol
        // glm::vec3 dbg_scoreMin(-3.7f, -1.0f, -14.3f); 
        // glm::vec3 dbg_scoreMax( 3.7f,  1.6f, -13.6f); 
        // DrawDebugAABB(dbg_scoreMin, dbg_scoreMax, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), view, projection);

        // ---------------------------------------------------------------
        // Renderiza a linha de trajetória de Bézier cúbica (apenas em
        // g_CameraState == CAM_BALL), mostrando o arco previsto para o chute da bola.
        // ---------------------------------------------------------------
        if (g_CameraState == CAM_BALL && !g_HasKicked)
        {
            // Atualiza (ou cria) o VAO/VBO com os pontos da curva
            UpdateBezierTrajectory();

            // Usa o programa de GPU minimalista (sem iluminação/textura)
            glUseProgram(g_BezierProgramID);

            // Envia as matrizes view e projection ao shader da linha
            GLint bezier_view_u = glGetUniformLocation(g_BezierProgramID, "view");
            GLint bezier_proj_u = glGetUniformLocation(g_BezierProgramID, "projection");
            glUniformMatrix4fv(bezier_view_u, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(bezier_proj_u, 1, GL_FALSE, glm::value_ptr(projection));

            // Desabilita o depth test para que a linha apareça sobre os objetos
            glDisable(GL_DEPTH_TEST);

            glLineWidth(6.0f);
            glUniform4f(glGetUniformLocation(g_BezierProgramID, "lineColor"), 1.0f, 0.85f, 0.0f, 1.0f);
            glBindVertexArray(g_BezierVAO);
            int pointsToDraw = g_ShowFullBezier ? (BEZIER_SEGMENTS + 1) : (BEZIER_SEGMENTS / 2 + 1);
            glDrawArrays(GL_LINE_STRIP, 0, pointsToDraw);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
            glLineWidth(1.0f);

            // Retorna ao programa de GPU principal
            glUseProgram(g_GpuProgramID);
        }
        // ---------------------------------------------------------------

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        
        // Cálculo do fator de escala baseado na altura da janela para ajustar tamanho do texto
        float scaleX = (float)currentWidth / 800.0f;
        float scaleY = (float)currentHeight / 600.0f;

        float dynamicScale = std::min(scaleX, scaleY);
        float finalScale = 1.5f * dynamicScale;
        float charWidth = TextRendering_CharWidth(window);

        // Telas de início, vitória e derrota
        if (g_HasKicked && g_TimeSinceKick >= 4.0f && !g_ShowWinScreen && !g_ShowGameOverScreen)
        {
            if (g_Score >= g_TargetGoals) {
                g_ShowWinScreen = true;
                g_CameraState = CAM_DEFAULT;
                g_CameraTheta = 0.0f;
                g_CameraPhi = 0.0f;
                g_CameraDistance = 3.5f;
            }
            else if (g_RemainingAttempts <= 0) {
                g_ShowGameOverScreen = true;
                g_CameraState = CAM_DEFAULT;
                g_CameraTheta = 0.0f;
                g_CameraPhi = 0.0f;
                g_CameraDistance = 3.5f;
            }
        }

        if (g_ShowStartScreen)
        {
            std::string startStr = "Aperte qualquer botao para iniciar";
            float textWidth = startStr.length() * charWidth * finalScale;
            TextRendering_PrintString(window, startStr, 0.0f - (textWidth / 2.0f), 0.0f, finalScale);
        }
        else if (g_ShowWinScreen)
        {
            std::string winStr = "VOCE VENCEU!";
            float winWidth = winStr.length() * charWidth * finalScale;
            TextRendering_PrintString(window, winStr, 0.0f - (winWidth / 2.0f), 0.1f, finalScale);

            std::string resetStr = "Aperte qualquer tecla para recomecar";
            float resetWidth = resetStr.length() * charWidth * finalScale;
            TextRendering_PrintString(window, resetStr, 0.0f - (resetWidth / 2.0f), -0.1f, finalScale);
        }
        else if (g_ShowGameOverScreen)
        {
            std::string goStr = "GAME OVER";
            float goWidth = goStr.length() * charWidth * finalScale;
            TextRendering_PrintString(window, goStr, 0.0f - (goWidth / 2.0f), 0.1f, finalScale);

            std::string resetStr = "Aperte qualquer tecla para tentar novamente";
            float resetWidth = resetStr.length() * charWidth * finalScale;
            TextRendering_PrintString(window, resetStr, 0.0f - (resetWidth / 2.0f), -0.1f, finalScale);
        }
        else
        {
            // Desenha a barra de força na tela do jogador
            if (g_BallState == BALL_POWER)
            {
                // Converte a força atual (0.8 a 1.7) para uma quantidade de barrinhas (0 a 20)
                // Intervalo é 0.9f
                int numBars = (int)(((g_KickPower - 0.8f) / 0.9f) * 20.0f);
                
                std::string powerStr = "FORCA: [";
                for(int i = 0; i < 20; i++) {
                    if (i < numBars) powerStr += "|";
                    else powerStr += " ";
                }
                powerStr += "]";
                
                float textWidth = powerStr.length() * charWidth * finalScale;
                float posX = 0.5f - (textWidth / 2.0f);

                // Imprime no centro inferior da tela
                TextRendering_PrintString(window, powerStr, posX, -0.7f, finalScale);
            }

            // Desenha o placar
            char scoreStr[50];
            snprintf(scoreStr, 50, "GOLS: %d / %d   TENTATIVAS: %d", g_Score, g_TargetGoals, g_RemainingAttempts);
            
            // Imprime no canto superior esquerdo da tela
            TextRendering_PrintString(window, scoreStr, -0.9f, 0.9f, finalScale);

            if (g_HasKicked && g_TimeSinceKick >= 4.0f)
            {
                std::string retryStr = "Pressione [Q] para o proximo chute";

                float textWidth = retryStr.length() * charWidth * finalScale;
                float posX = 0.0f - (textWidth / 2.0f);

                TextRendering_PrintString(window, retryStr, posX, 0.0f, finalScale);
            }
        }


        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);

    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;
    
        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = M_PI_2; 
        float phimin = -phimax;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (g_CameraState == CAM_BALL && g_BallState == BALL_IDLE)
    {
        // Em modo g_CameraState == CAM_BALL o scroll controla o pico do arco de Bézier.
        // Scroll up aumenta, scroll down diminui
        g_BezierArcHeight += 0.5f * (float)yoffset;

        // Arco minimo e máximo
        if (g_BezierArcHeight < 0.0f)  g_BezierArcHeight = 0.0f;
        if (g_BezierArcHeight > 5.0f) g_BezierArcHeight = 5.0f; // quanto a gente tiver a barreira podemos testar o valor maximo pra ficar bom
        return;
    }

    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    // Tela inicial, vitória ou derrota
    if (g_ShowStartScreen || g_ShowWinScreen || g_ShowGameOverScreen)
    {
        if (action == GLFW_PRESS)
        {
            if (g_ShowStartScreen) {
                g_ShowStartScreen = false; 
                g_CameraState = CAM_AERIAL; 
                g_CameraPhi = M_PI_2;
                g_CameraDistance = 25.0f;
            } else {
                ResetGame(); 
            }
        }
        return; 
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla enter, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Se o usuário apertar a tecla T, alterna entre a mira cheia e a mira pela metade
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        g_ShowFullBezier = !g_ShowFullBezier;
    }

    // Tecla F cicla entre os estados da câmera:
    //   CAM_DEFAULT → CAM_AERIAL → CAM_BALL → CAM_DEFAULT
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        switch (g_CameraState)
        {
        case CAM_DEFAULT:
            // Salva câmera atual e entra em visão aérea
            g_PrevCameraTheta    = g_CameraTheta;
            g_PrevCameraPhi      = g_CameraPhi;
            g_PrevCameraDistance = g_CameraDistance;
            g_CameraTheta    = 0.0f;
            g_CameraPhi      = M_PI_2;
            g_CameraDistance = 25.0f;
            g_CameraState = CAM_AERIAL;
            break;

        case CAM_AERIAL:
            // Mantém os parâmetros esféricos do aéreo e entra em visão da bola
            g_CameraState = CAM_WALL;
            break;
        
        case CAM_WALL:
            g_CameraState = CAM_BALL;
            break;

        case CAM_BALL:
            // Restaura câmera original e volta ao estado padrão
            g_CameraTheta    = g_PrevCameraTheta;
            g_CameraPhi      = g_PrevCameraPhi;
            g_CameraDistance = g_PrevCameraDistance;
            g_CameraState = CAM_DEFAULT;
            break;
        }
    }

    // Movimento da bola com WASD quando em visão aérea (top-down).
    if (g_CameraState == CAM_AERIAL && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        float step = 0.2f; // valor de deslocamento 
        if (key == GLFW_KEY_W)
            g_BallPosZ -= step;
        if (key == GLFW_KEY_S)
            g_BallPosZ += step;
        if (key == GLFW_KEY_A)
            g_BallPosX -= step;
        if (key == GLFW_KEY_D)
            g_BallPosX += step;
    }

    // Ajuste do ponto final da curva de Bézier com WASD em g_CameraState == CAM_BALL.
    // Teclas controlam a posição X e Y de P3.
    // acho que vai ficar muito dificil sem esses limites, testar depois
    if (g_CameraState == CAM_BALL && g_BallState == BALL_IDLE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        float step = 0.2f;
        if (key == GLFW_KEY_W)
        {
            g_BezierTargetY += step;
            if (g_BezierTargetY > 10.0f) g_BezierTargetY = 10.0f; // altura máxima pra nao dar um bagão
        }
        if (key == GLFW_KEY_S)
        {
            g_BezierTargetY -= step;
            if (g_BezierTargetY < 0.0f) g_BezierTargetY = 0.0f;  // chão
        }
        if (key == GLFW_KEY_A)
        {
            g_BezierTargetX -= step;
            if (g_BezierTargetX < -6.0f) g_BezierTargetX = -6.0f; // maximo da esquerda
        }
        if (key == GLFW_KEY_D)
        {
            g_BezierTargetX += step;
            if (g_BezierTargetX >  6.0f) g_BezierTargetX =  6.0f; // maximo da direita
        }
    }

// Chutar a bola ao pressionar ESPAÇO
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        if (g_CameraState == CAM_AERIAL)
        {
            g_CameraState = CAM_BALL;
        }
        // Sai do idle e trava a mira
        else if (g_CameraState == CAM_BALL && g_BallState == BALL_IDLE && !g_HasKicked)
        {
            g_BallState = BALL_POWER;
            g_KickPower = 0.8f;      
            g_PowerDirection = 1.0f; 

            // Salva os pontos da curva para travar o caminho
            g_P0 = glm::vec3(g_BallPosX, g_BallPosY, g_BallPosZ);
            g_P3 = glm::vec3(g_BezierTargetX, g_BezierTargetY, -13.0f);
            
            glm::vec3 dir = g_P3 - g_P0;
            g_P1 = g_P0 + dir * 0.25f + glm::vec3(0.0f, g_BezierArcHeight, 0.0f);
            g_P2 = g_P0 + dir * 0.75f + glm::vec3(0.0f, g_BezierArcHeight * 0.67f, 0.0f);
        }
        // Trava a força que estiver na barra e executa o chute
        else if (g_BallState == BALL_POWER)
        {
            g_HasKicked = true; 
            g_BallState = BALL_BEZIER;
            g_KickTime_t = 0.0f;
            g_RemainingAttempts--;
        }
    }


    // if (g_CameraState == CAM_WALL && (action == GLFW_PRESS || action == GLFW_REPEAT))
    // {
    //     float step = 0.2f;
    //     if (key == GLFW_KEY_D)
    //     {
    //         g_WallOffsetX -= step;
    //         if (g_WallOffsetX < -6.0f) g_WallOffsetX = -6.0f; // maximo da esquerda
    //     }
    //     if (key == GLFW_KEY_A)
    //     {
    //         g_WallOffsetX += step;
    //         if (g_WallOffsetX >  6.0f) g_WallOffsetX =  6.0f; // maximo da direita
    //     }
    // }

    // Reinicia o jogo (Hard Reset)
    if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
    {
        ResetGame();
    }

    // Próxima tentativa (Soft Reset)
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        if (g_HasKicked && g_TimeSinceKick >= 4.0f)
        {
            ResetTurn();
        }
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :