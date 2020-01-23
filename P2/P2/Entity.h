#pragma once
#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"


class Entity {
public:
    
    glm::vec3 position;
    glm::vec3 move;
    //glm::vec3 acceleration;
    
    float speed;
    
    GLuint textureID;
    
    Entity();
    
    void UpdatePos(const float incrementX, const float incrementY);
    void Update(float deltaTime);
    void Render(ShaderProgram *program, glm::vec3 sizing);
};



