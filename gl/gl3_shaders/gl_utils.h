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
#include <vector>

///////////////////////////////////
//////// GL_UTILS HEADER //////////
///////////////////////////////////

// func prototypes
bool init();					// Starts up SDL, creates window, and initializes OpenGL
bool initGL();					// Initializes matrices and clear color
void handleKeys( 
	unsigned char key, 
	int x, 
	int y );					// Input handler
void update( float delta );		// Per frame update
void renderQuad();				// Renders a flat quad to fill the screen
void render();					// Renders quad to the screen
void close();					// Frees media and shuts down SDL
void shaderSendMatrix( unsigned int location, glm::mat4 &matrix );
void setMat4(unsigned int &ID, const std::string &name, const glm::mat4 &mat);
void shaderSendMatrix( unsigned int location, glm::mat3 &matrix );
void setViewport();
glm::mat3 getNormalMatrix( glm::mat4 inMatrix );

// shader stuff
void printProgramLog( GLuint program );
void printShaderLog( GLuint shader );
GLuint loadShaderFromFile( std::string path, GLenum shaderType );
bool loadProgram(GLuint &id, std::string vertSource, std::string fragSource, std::string geoSource="" );
void setColor( GLint &location, GLfloat r, GLfloat g, GLfloat b );
GLuint loadTexFromFile( const char *filename, unsigned int width, unsigned int height, bool gammaCorrection, bool filtering );

#endif
