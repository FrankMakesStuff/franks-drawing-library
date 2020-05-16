/*
	Name: "gl_utils.h"
	Copyright: 
	Author: Frank
	Date: 15/05/20
	Description: 		header file for GL stuff
*/

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>


// func prototypes
bool init();					// Starts up SDL, creates window, and initializes OpenGL
bool initGL();					// Initializes matrices and clear color
void handleKeys( 
	unsigned char key, 
	int x, 
	int y );					// Input handler
void update();					// Per frame update
void render();					// Renders quad to the screen
void close();					// Frees media and shuts down SDL

// shader stuff
void printProgramLog( GLuint program );
void printShaderLog( GLuint shader );
GLuint loadShaderFromFile( std::string path, GLenum shaderType );
bool loadProgram(GLuint &id);
void setColor( GLint &location, GLfloat r, GLfloat g, GLfloat b );

#endif
