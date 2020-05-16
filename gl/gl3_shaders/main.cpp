// these preprocessor instructions are needed by SDL2 and glew32 to compile successfully
#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include "gl_utils.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=- DEFINITIONS -=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TEX_DIMENSION = 256;

SDL_Window* gWindow = NULL;		// The window we'll be rendering to
SDL_GLContext gContext;			// OpenGL context
bool gRenderQuad = true;		// Render flag

// shader globals
GLuint gProgramID = 0;

GLint gVertexPos2DLocation = -1;
GLint gColorLocation = -1;

GLuint gVBO = 0;
GLuint gIBO = 0;
GLuint gVAO = 0;

glm::mat4 view( 1.0f );
glm::mat4 proj( 1.0f );
glm::mat4 model( 1.0f );

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
        //Use OpenGL 3.1 to use shaders
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

        //Create window
        printf("SUCCESS: SDL2 started...\n");
        gWindow = SDL_CreateWindow( "Shader Demo", 
									SDL_WINDOWPOS_UNDEFINED, 
									SDL_WINDOWPOS_UNDEFINED, 
									SCREEN_WIDTH, 
									SCREEN_HEIGHT, 
									SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
        if( gWindow == NULL ){
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        } else {
        	printf("SUCCESS: Window created %d x %d...\n", SCREEN_WIDTH, SCREEN_HEIGHT );
            //Create context
            gContext = SDL_GL_CreateContext( gWindow );
            if( gContext == NULL ){
                printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            } else {
            	printf( "SUCCESS: OpenGL context created.\n" );
            	// initialize GLEW
            	glewExperimental = GL_TRUE;
            	GLenum glewError = glewInit();
            	if( glewError != GLEW_OK ) printf( "Error initializing GLEW: %s\n", glewGetErrorString( glewError ) );
            	
                //Use Vsync
                if( SDL_GL_SetSwapInterval( 1 ) < 0 )
                    printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );

                //Initialize OpenGL
                if( !initGL() ){
                    printf( "Unable to initialize OpenGL!\n" );
                    success = false;
                } printf( "SUCCESS: OpenGL started...\n" );
            }
        }
    }
    
    // initialize SDL image helper library
    if( IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG != IMG_INIT_PNG  ){
		SDL_Quit();
		success = false;	
    }
    
    printf( "SUCCESS: SDL Image started...\n" );
    return success;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=- InitGL -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool initGL(){
    bool success = true;
    GLenum error = GL_NO_ERROR;
    
    // enable z-buffer
    glEnable( GL_DEPTH_TEST );
    
	// load default program
	if( loadProgram( gProgramID ) == false ){
		printf( "Loading shader program failed!\n" );
		success = false;
	}
	else
    {
        // Lets start with initting basic GL stuff
        glClearColor( 0.1f, 0.2f, 0.3f, 1.0f );

        //VBO data
        float vertexData[] =
        {
            // positions         // colors          // uv coords
		     5.f, -5.f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,   // bottom right
		    -5.f, -5.f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,   // bottom left
		     0.0f,  5.f, 0.0f,  0.0f, 1.0f, 0.0f,  0.5f, 0.0f    // top 
        };

        //Create Vertex Buffer Object, and send our vertex data and specifications to OpenGL
        glGenVertexArrays( 1, &gVAO );
        glBindVertexArray( gVAO );
        
        glGenBuffers( 1, &gVBO );
        glBindBuffer( GL_ARRAY_BUFFER, gVBO );
        glBufferData( GL_ARRAY_BUFFER, sizeof( vertexData ), vertexData, GL_STATIC_DRAW );
        
        // REFERENCE: glVertexAttribPointer( unsigned int index, int size (only 1-4 allowed), data-type, needs-normalized, offset-pointer )
        
		// position attribute
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0 );
		glEnableVertexAttribArray(0);
		// color attribute
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)) );
		glEnableVertexAttribArray(1);
		// texture attribute
		glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)) );
		glEnableVertexAttribArray(2);
		
		// initialize matrices
		view = glm::translate( view, glm::vec3( 0.0f, 0.0f, -8.0f ) );
		proj = glm::perspective( glm::radians( 45.0f ), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f );
		
		glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	}

    return success;
}

void update(){
	float theta = 0.5f;
	
	// rotate the model matrix (note: this applies a rotation each frame, causing the triangle to spin during the loop)
	model = glm::rotate( model, glm::radians( theta ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	
	// send off all matrix info to vertex shader
	int modelLoc = glGetUniformLocation( gProgramID, "model" );
	glUniformMatrix4fv( modelLoc, 1, GL_FALSE, glm::value_ptr( model ) );	
	int viewLoc = glGetUniformLocation( gProgramID, "view" );
	glUniformMatrix4fv( viewLoc, 1, GL_FALSE, glm::value_ptr( view ) );
	int projLoc = glGetUniformLocation( gProgramID, "projection" );
	glUniformMatrix4fv( projLoc, 1, GL_FALSE, glm::value_ptr( proj ) );
	
	
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=- render -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void render()
{
    //Clear color buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    //Render quad
    if( gRenderQuad )
    {
        glDrawArrays( GL_TRIANGLES, 0, 3 );
    }
}

void close(){
	glDeleteProgram( gProgramID );
	printf( "CLOSED: GL shader program...\n" );
	SDL_DestroyWindow( gWindow );
	printf( "CLOSED: GL window...\n" );
	SDL_GL_DeleteContext( gContext );
	printf( "CLOSED: GL context...\n" );
	IMG_Quit();
	printf( "CLOSED: SDL Image...\n" );
	SDL_Quit();	
	printf( "CLOSED: SDL2...\n");
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
	
	// bind our shader program (we only have one)
	glUseProgram( gProgramID );
	
	std::string imagePath = SDL_GetBasePath() + (std::string)"mustard.png";
	printf("Loading texture %s...\n", imagePath.c_str() );
	SDL_Surface *tex = IMG_Load( imagePath.c_str() );
	if ( tex == nullptr ){
		close();
		printf( "ERROR: Could not load texture - %s\n", SDL_GetError() );
		return 1;
	} else {
		
		glEnable( GL_TEXTURE_2D );
		unsigned int tex_id;
		glGenTextures( 1, &tex_id );
		glBindTexture( GL_TEXTURE_2D, tex_id );
		
		 // filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ); // GL_MODULATE
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ); // GL_REPEAT, GL_CLAMP
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		
		int mode = GL_RGBA;
		// DEBUG TEST - GET RID OF IT WHEN DONE
		float pixels[] = {
			1.0f, 1.0f, 1.0f,	0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f,	1.0f, 1.0f, 1.0f
		};
		
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels );									// test/checkerboard pixels
		glTexImage2D( GL_TEXTURE_2D, 0, mode, TEX_DIMENSION, TEX_DIMENSION, 0, mode, GL_UNSIGNED_BYTE, tex->pixels);	// real image
		glGenerateMipmap( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, tex_id );
		
		GLenum err = GL_NO_ERROR;
		err = glGetError();
		if( err != GL_NO_ERROR )
			printf( "ERROR: GL could not create texture - %s\n", gluErrorString( err ) );
		
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
                quit = true;
        }
        // update scene
        update();
        
        // render scene
        render();
        
        // draw to screen
        SDL_GL_SwapWindow( gWindow );
	} printf( "Exiting loop, cleaning up...\n" );

	// exit the shader program
	glUseProgram( 0 );
	
    // clean up
    close();
    
    printf( "Quitting...\n" );
	return 0;	
}


