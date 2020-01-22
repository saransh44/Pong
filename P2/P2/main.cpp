#define GL_SILENCE_DEPRECATION //For silencing pesky notifications on Mac

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram plain;
ShaderProgram textured;
glm::mat4 viewMatrix, projectionMatrix;


//We will indicate winning with the texture as well as the color change in homage to the classic pong
GLuint winID;

Entity rightPaddle, leftPaddle, ball;
glm::vec3 paddleSizing = glm::vec3(0.3f, 1.2f, 0.0f);
glm::vec3 ballSizing = glm::vec3(0.2f, 0.2f, 0.0f);


float ballPosIncrementX = -0.002;
float ballPosIncrementY = -0.002;


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

float xDistanceFromPad1 = 0;
float yDistanceFromPad1 = 0;

float xDistanceFromPad2 = 0;
float yDistanceFromPad2 = 0;



//janky collision detection Woohoo
void totallyNotFakeCollisionProtocol() {

    xDistanceFromPad1 = fabs(ball.position.x - leftPaddle.position.x);
    yDistanceFromPad1 = fabs(ball.position.y - leftPaddle.position.y);

    if (xDistanceFromPad1 < 0.05 && yDistanceFromPad1 < 0.65) {
        ballPosIncrementX *= -1;
    }

    xDistanceFromPad2 = fabs(ball.position.x - rightPaddle.position.x);
    yDistanceFromPad2 = fabs(ball.position.y - rightPaddle.position.y);

    std::cout << yDistanceFromPad2 << std::endl;

    if(xDistanceFromPad2 < 0.05 && yDistanceFromPad2 < 0.65) {
        ballPosIncrementX *= -1;
    }

    if (ball.position.y > 3.6) {
        ballPosIncrementY *= -1;

    }

    if (ball.position.y < -3.6) {
        ballPosIncrementY *= -1;
    }


    if (ball.position.x > 4.80) {
        ballPosIncrementX = 0.0;
        ballPosIncrementY = 0.0;
    }
        
    if (ball.position.x < -4.80) {
        ballPosIncrementX = 0.0;
        ballPosIncrementY = 0.0;
    }
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    plain.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::mat4(1.0f); //modelMatrix is used elegantly in Entity.cpp
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    plain.SetProjectionMatrix(projectionMatrix);
    plain.SetViewMatrix(viewMatrix);
    plain.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

    glUseProgram(plain.programID);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //2rightPaddle.speed = 1.0f;
    rightPaddle.position = glm::vec3(4.25, 0, 0);

    //leftPaddle.speed = 1.0f;
    leftPaddle.position = glm::vec3(-4.25, 0, 0);

    //ball.speed = 3.5f;
    ball.position = glm::vec3(0, 0, 0);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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

    ball.position.x += ballPosIncrementX;
    ball.position.y += ballPosIncrementY;
    //leftPaddle.Update(deltaTime);
    //rightPaddle.Update(deltaTime);
    //ball.Update(deltaTime);
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    rightPaddle.Render(&plain, paddleSizing);
    leftPaddle.Render(&plain, paddleSizing);
    ball.Render(&plain, ballSizing);

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

