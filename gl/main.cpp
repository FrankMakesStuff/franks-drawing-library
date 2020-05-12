#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <GL/GLU.h>
#include <stdio.h>
#include <string>
#include <iostream>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=- DEFINITIONS -=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TEX_DIMENSION = 512;

bool init();					// Starts up SDL, creates window, and initializes OpenGL
bool initGL();					// Initializes matrices and clear color
void handleKeys( 
	unsigned char key, 
	int x, 
	int y );					// Input handler
void update();					// Per frame update
void render();					// Renders quad to the screen
void close();					// Frees media and shuts down SDL

SDL_Window* gWindow = NULL;		// The window we'll be rendering to
SDL_GLContext gContext;			// OpenGL context
bool gRenderQuad = true;		// Render flag

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-  INIT -=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool init(){
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    } else{
        //Use OpenGL 2.1
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

        //Create window
        gWindow = SDL_CreateWindow( "SDL Demo", 
									SDL_WINDOWPOS_UNDEFINED, 
									SDL_WINDOWPOS_UNDEFINED, 
									SCREEN_WIDTH, 
									SCREEN_HEIGHT, 
									SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
        if( gWindow == NULL ){
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        } else {
            //Create context
            gContext = SDL_GL_CreateContext( gWindow );
            if( gContext == NULL ){
                printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            } else {
                //Use Vsync
                if( SDL_GL_SetSwapInterval( 1 ) < 0 )
                    printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );

                //Initialize OpenGL
                if( !initGL() ){
                    printf( "Unable to initialize OpenGL!\n" );
                    success = false;
                }
            }
        }
    }
    
    // initialize SDL image helper library
    if( IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG != IMG_INIT_PNG  ){
		SDL_Quit();
		success = false;	
    }
    
    return success;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=- InitGL -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool initGL(){
    bool success = true;
    GLenum error = GL_NO_ERROR;

    //Initialize Projection Matrix
    glViewport( 0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0, (GLfloat)SCREEN_WIDTH/(GLfloat)SCREEN_HEIGHT, 1.0, 200.0 );
    
    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR ){
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }

    //Initialize Modelview Matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR ){
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }
    
   //Initialize clear color
    glClearColor( 0.5f, 0.5f, 0.5f, 1.f );
    
    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR ){
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }
    
    return success;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=- handleKeys -=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void handleKeys( unsigned char key, int x, int y ){
    //Toggle quad
    if( key == 'q' ){
        gRenderQuad = !gRenderQuad;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=- update -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void update(){
    static float theta = 0.0f;
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0, 0.0, -15.0 );
    glRotatef( theta, 1.0f, 1.0f, 1.0f );
    
    theta += 0.75f;
    if( theta > 360.0 ) theta -= 360.0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=- render -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void render(GLuint &id){
    //Clear color buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // enables
    glEnable( GL_TEXTURE_2D );

    // color
    glColor3f( 1.0f, 1.0f, 1.0f );
    
    glBindTexture( GL_TEXTURE_2D, id );
    
    //Render quad
    if( gRenderQuad ){
        glBegin( GL_QUADS );
            glTexCoord2f(0.0, 1.0); glVertex2f( -5.0f, -5.0f );
            glTexCoord2f(1.0, 1.0); glVertex2f( 5.0f, -5.0f );
            glTexCoord2f(1.0, 0.0); glVertex2f( 5.0f, 5.0f );
            glTexCoord2f(0.0, 0.0); glVertex2f( -5.0f, 5.0f );
        glEnd();
    }
}

void close(){
	SDL_DestroyWindow( gWindow );
	SDL_GL_DeleteContext( gContext );
	IMG_Quit();
	SDL_Quit();	
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=- main -=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int main() {
	bool quit = false;
	SDL_Event e;

	// Init SDL
	if( !init() ) return 1;
	
	// Init OpenGL
	if( !initGL() ) return 1;
	
	//Enable text input
    SDL_StartTextInput();
	
	// Load a texture
	GLuint TextureID;
	
	std::string imagePath = SDL_GetBasePath() + (std::string)"crate.png";
	std::cout << "Attempting to load file: " << imagePath << std::endl;
	SDL_Surface *tex = IMG_Load(imagePath.c_str());
	
	if ( tex == nullptr ){
		close();
		std::cout << "Texture load Error: " << SDL_GetError() << std::endl;
		return 1;
	} else {
		glGenTextures( 1, &TextureID );
		glBindTexture( GL_TEXTURE_2D, TextureID );
		
		 // filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ); // GL_MODULATE
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ); // GL_REPEAT, GL_CLAMP
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		
		glEnable( GL_DEPTH_TEST );
		
		GLfloat fog_density = 0.08f;
		GLfloat fog_color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		glEnable( GL_FOG );
		glFogi( GL_FOG_MODE, GL_EXP2 );
		glFogfv( GL_FOG_COLOR, fog_color );
		glFogf( GL_FOG_DENSITY, fog_density );
		glHint( GL_FOG_HINT, GL_NICEST );
		
		// DEBUG TEST - GET RID OF IT WHEN DONE
		float pixels[] = {
			1.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f,	1.0, 0.0f, 1.0f
		};
		
		int mode = GL_RGBA;
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels );									// test/checkerboard pixels
		glTexImage2D( GL_TEXTURE_2D, 0, mode, TEX_DIMENSION, TEX_DIMENSION, 0, mode, GL_UNSIGNED_BYTE, tex->pixels);	// real image
		std::cout << glGetError();
		
		if( tex ) SDL_FreeSurface( tex );
	}

    //While application is running
    while( !quit )
    {
        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
            //Handle keypress with current mouse position
            else if( e.type == SDL_TEXTINPUT )
            {
                int x = 0, y = 0;
                SDL_GetMouseState( &x, &y );
                handleKeys( e.text.text[ 0 ], x, y );
            }
        }
		// update scene
		update();
		
        //Render quad
        render( TextureID );
        
        //Update screen
        SDL_GL_SwapWindow( gWindow );
    }
    
    //Disable text input
    SDL_StopTextInput();
    
    // free the gl texture
    glDeleteTextures( 1, &TextureID );
    
    // clean up
    close();
	return 0;	
}


