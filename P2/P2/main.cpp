#define GL_SILENCE_DEPRECATION //For silencing pesky notifications on Mac

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ctime> 

#include<stdlib.h> //why does srand compile if you comment this out? weird.
//#include<stdio.h>

//using namespace std;
SDL_Window* displayWindow;
bool gameIsRunning = true;

//ShaderProgram plain;
ShaderProgram textured;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

//I will indicate immediate wins for the grading criteria using the font1 that is used in future projects
GLuint textID;

Entity rightPaddle, leftPaddle, ball;
glm::vec3 paddleSizing = glm::vec3(0.3f, 1.2f, 0.0f);
glm::vec3 ballSizing = glm::vec3(0.2f, 0.2f, 0.0f);

ShaderProgram unTextured;

float ballPosIncrementX;
float ballPosIncrementY;

bool leftWins = false;
bool rightWins = false;
GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}
void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {
        int index = (int)text[i];

        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;


        texCoords.insert(texCoords.end(), { u, v + height, u + width, v + height, u + width, v, u, v + height, u + width, v, u, v });

        float offset = (size + spacing) * i;
        vertices.insert(vertices.end(), { offset + (-0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (0.5f * size),
                                        offset + (-0.5f * size), (-0.5f * size),
                                        offset + (0.5f * size), (0.5f * size),
                                        offset + (-0.5f * size), (0.5f * size) });
    }

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);


    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2.0f);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

float xDistanceFromPad1 = 0;
float yDistanceFromPad1 = 0;

float xDistanceFromPad2 = 0;
float yDistanceFromPad2 = 0;

const float padCollisionWidth = .15;
const float padCollisionHeight = .7;

void Initialize() {
    srand(time(0)); //need some kind of randomness so not every game starts out the same way 
    ballPosIncrementX = (rand() % 13 / 10000.0) + 0.001;
    ballPosIncrementY = (rand() % 13 / 10000.0) + 0.001;
    //std::cout << ballPosIncrementY << std::endl;

    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    //plain.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::mat4(1.0f); //modelMatrix is used elegantly in Entity.cpp
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    unTextured.Load("shaders/vertex.glsl", "shaders/fragment.glsl");

    unTextured.SetProjectionMatrix(projectionMatrix);
    unTextured.SetViewMatrix(viewMatrix);
    unTextured.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    glUseProgram(unTextured.programID);

    //plain.SetModelMatrix(modelMatrix);

    textured.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    textID = LoadTexture("font1.png");

    textured.SetProjectionMatrix(projectionMatrix);
    textured.SetViewMatrix(viewMatrix);
    textured.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

    glUseProgram(textured.programID);

    //glUseProgram(plain.programID);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    rightPaddle.position = glm::vec3(4.25, 0, 0);

    leftPaddle.position = glm::vec3(-4.25, 0, 0);

    ball.position = glm::vec3(0, 0, 0);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

//janky collision detection Woohoo
void totallyNotFakeCollisionProtocol() {

    xDistanceFromPad1 = fabs(ball.position.x - leftPaddle.position.x);
    yDistanceFromPad1 = fabs(ball.position.y - leftPaddle.position.y);

    if (xDistanceFromPad1 < padCollisionWidth && yDistanceFromPad1 < padCollisionHeight) {
        ballPosIncrementX *= -1;
    }

    xDistanceFromPad2 = fabs(ball.position.x - rightPaddle.position.x);
    yDistanceFromPad2 = fabs(ball.position.y - rightPaddle.position.y);

    //std::cout << xDistanceFromPad2 << std::endl;

    if(xDistanceFromPad2 < padCollisionWidth && yDistanceFromPad2 < padCollisionHeight) {
        ballPosIncrementX *= -1;
    }

    if (ball.position.y > 3.6) {
        ballPosIncrementY *= -1;
    }

    if (ball.position.y < -3.6) {
        ballPosIncrementY *= -1;
    }

    //Win/lose conditionals
    if (ball.position.x > 4.80) {
        ballPosIncrementX = 0.0;
        ballPosIncrementY = 0.0;

        leftWins = true;

    }
        
    if (ball.position.x < -4.80) {
        ballPosIncrementX = 0.0;
        ballPosIncrementY = 0.0;
        
        rightWins = true;
    }
}



void ProcessInput() {
    //player_movement = glm::vec3(0, 0, 0);


    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                // Some sort of action
                break;

            }
            break;
        }
    }

    // Check for pressed/held keys below
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W] && leftPaddle.position.y < 3.15)
    {
        leftPaddle.position.y += .003;
    }
    else if (keys[SDL_SCANCODE_S] && leftPaddle.position.y > -3.15)
    {
        leftPaddle.position.y += -.003;
    }

    if (keys[SDL_SCANCODE_UP] && rightPaddle.position.y < 3.15)
    {
        rightPaddle.position.y += .003;
    }
    else if (keys[SDL_SCANCODE_DOWN] && rightPaddle.position.y > -3.15)
    {
        rightPaddle.position.y += -.003;
    }

    //if (glm::length(player_move) > 1.0f)
    //{
    //    player_move = glm::normalize(player_move);
    //}
}

float lastTicks = 0;
float rotate_z = 0;


void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    totallyNotFakeCollisionProtocol();

    //ball.position.x += ballPosIncrementX;
    //ball.position.y += ballPosIncrementY;

    ball.UpdatePos(ballPosIncrementX, ballPosIncrementY);
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    //DrawText(&textured, textID, "Left Wins!", 0.3, -0.2, glm::vec3(-4.7, 3.5, 0));
    if (leftWins) {
        std::cout << "Left Player is the winner! You will find your prize up at the front desk, champ!" << std::endl;
        DrawText(&textured, textID, "Left Wins!", 0.3, -0.2, glm::vec3(-4.7, 3.5, 0));
        //gameIsRunning = false; //up to you if you wanna bask in the glory of your win screen or exit immediatly 

    }
    if (rightWins) {
        std::cout << "Right Player is the winner! You will find your prize up at the front desk, champ!" << std::endl;
        DrawText(&textured, textID, "Right Wins!", 0.3, -0.2, glm::vec3(3.8, 3.5, 0));
        //gameIsRunning = false;
    }

    rightPaddle.Render(&unTextured, paddleSizing);
    leftPaddle.Render(&unTextured, paddleSizing);
    ball.Render(&unTextured, ballSizing);

    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}

