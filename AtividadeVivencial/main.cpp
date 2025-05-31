// main.cpp
// Jogo Parallax com camadas e personagem fixo

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int LARGURA = 800;
const int ALTURA = 600;
float cameraX = 0.0f; // offset da "camera" para efeito parallax

// Estrutura de camada com textura + velocidade relativa
struct Camada {
    GLuint textura;
    float velocidade; // 0.0 = infinito longe, 1.0 = perto
};

std::vector<Camada> camadas;
GLuint shaderProgram;
GLuint quadVAO;

// Shaders simples para desenhar uma textura com transform
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 model;

out vec2 TexCoord;
void main() {
    gl_Position = model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTex;
})";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textura;
void main() {
    FragColor = texture(textura, TexCoord);
})";

GLuint carregarTextura(const char* caminho) {
    GLuint texturaID;
    glGenTextures(1, &texturaID);
    glBindTexture(GL_TEXTURE_2D, texturaID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int largura, altura, canais;
    unsigned char* dados = stbi_load(caminho, &largura, &altura, &canais, 4);
    if (dados) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largura, altura, 0, GL_RGBA, GL_UNSIGNED_BYTE, dados);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Erro ao carregar textura: " << caminho << std::endl;
    }
    stbi_image_free(dados);
    return texturaID;
}

GLuint compilarShader(GLenum tipo, const char* src) {
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

void inicializarShaders() {
    GLuint vs = compilarShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compilarShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void inicializarQuad() {
    float quad[] = {
        //  x     y     u    v
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,

         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f
    };

    GLuint VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void renderizarCamada(Camada& camada) {
    glBindTexture(GL_TEXTURE_2D, camada.textura);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-cameraX * camada.velocidade, 0.0f, 0.0f));

    GLuint locModel = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void processarInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraX -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraX += 0.01f;
}

int main() {
    stbi_set_flip_vertically_on_load(true);
    glfwInit();
    GLFWwindow* janela = glfwCreateWindow(LARGURA, ALTURA, "Parallax Game", NULL, NULL);
    glfwMakeContextCurrent(janela);
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    inicializarShaders();
    inicializarQuad();

    camadas.push_back({ carregarTextura("PNG/game_background_3/layers/sky.png"), 0.1f });
    camadas.push_back({ carregarTextura("PNG/game_background_3/layers/clouds_1.png"), 0.3f });
    camadas.push_back({ carregarTextura("PNG/game_background_3/layers/clouds_2.png"), 0.5f });
    camadas.push_back({ carregarTextura("PNG/game_background_3/layers/ground_1.png"), 1.0f });

    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);

    while (!glfwWindowShouldClose(janela)) {
        processarInput(janela);

        glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (Camada& c : camadas) {
            renderizarCamada(c);
        }

        glfwSwapBuffers(janela);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
