// these preprocessor instructions are needed by SDL2 and glew32 to compile successfully
#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include "gl_utils.h"
#include "frank_console.h"

/////////////////////
///// MAIN.CPP //////
/////////////////////

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=- DEFINITIONS -=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-
const int SCREEN_WIDTH 		= 800;
const int SCREEN_HEIGHT 	= 600;
const int TEX_DIMENSION 	= 256;
const int LORES_WIDTH		= 320;
const int LORES_HEIGHT		= 200;
const float GODRAYS_SCALE	= 0.25f;

const bool USE_LORES = false;

const float CUBE_SIZE = 5.0f;
const float FLOOR_SIZE = 100.0f;
const float FLOOR_HEIGHT = 10.0f;

const unsigned int SHADOW_RES = 1024; // shadow map resolution - smaller=blockier/pixellated

bool DRAW_CUBE = true;
bool DRAW_FLOOR = true;
bool MOVE_LIGHT = true;
bool BLOOM = true;
float EXPOSURE = 1.0f;
unsigned int BlurAmount = 10;
float lightDistance = 20.0f;

SDL_Window* gWindow = NULL;
SDL_GLContext gContext;

// shader programs
GLuint gProgramID = 0;
GLuint gScreenProgram = 0;
GLuint gBlurProgram = 0;
GLuint gBloomProgram = 0;
GLuint gShadowProgram = 0;
GLuint gRaysProgram = 0;

// location info
GLint gVertexPos2DLocation = -1;
GLint gColorLocation = -1;

// scene objects
GLuint gTex = 0;
GLuint gFloortex = 0;

GLuint gVAO = 0;
GLuint gVBO = 0;
GLuint gEBO = 0;

GLuint gVAOfloor = 0;
GLuint gVBOfloor = 0;
GLuint gEBOfloor = 0;
glm::mat4 matFloor( 1.0f );

glm::vec3 lightPos( -1.f, 3.f, 4.f );
glm::vec3 viewPos( 0.0f, 0.0f, 50.0f );

// offscreen rendering objects
GLuint gFBO = 0; 				// main offscreen FBO
GLuint gColorBuffers[2];    	// 0=normal scene, 1=bright colors only
GLuint gRBO = 0;				// Render buffer object - holds depth and stencil info

// Gaussian blur
GLuint gBlurFBOs[2];			// Gaussian blur FBOs
GLuint gBlurColorBuffers[2];

// Low-Resolution low-color effect
GLuint gLoresFBO = 0;
GLuint gLoresColorBuffer = 0;

// shadow mapping
GLuint gShadowFBO = 0;
GLuint gShadowBuffer = 0; // a depth buffer, to be precise

// God Rays
GLuint gRaysFBO = 0;
GLuint gRaysBuffer = 0;

// offscreen geometry objects
GLuint VAO_screen = 0;
GLuint VBO_screen = 0;
GLuint EBO_screen = 0;

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
        printf( "ERROR: SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
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
            printf( "ERROR: Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        } else {
        	printf("SUCCESS: Window created %d x %d...\n", SCREEN_WIDTH, SCREEN_HEIGHT );
            //Create context
            gContext = SDL_GL_CreateContext( gWindow );
            if( gContext == NULL ){
                printf( "ERROR: OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            } else {
            	printf( "SUCCESS: OpenGL context created....\n" );
            	// initialize GLEW
            	glewExperimental = GL_TRUE;
            	GLenum glewError = glewInit();
            	if( glewError != GLEW_OK ) printf( "ERROR: Could not initialize GLEW: %s\n", glewGetErrorString( glewError ) );
            	
                //Use Vsync
                if( SDL_GL_SetSwapInterval( 1 ) < 0 )
                    printf( "WARNING: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
				
				// SDL is good here, now try and start OpenGL
                if( !initGL() ){
                    printf( "ERROR: Unable to initialize OpenGL!\n" );
                    success = false;
                }
            }
        }
    }
    
    // SDL image helper library - for loading textures from disk
    if( IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG != IMG_INIT_PNG  ){
		SDL_Quit();
		printf( "ERROR: SDL Image could not be initialized!\n" );
		success = false;	
    }

    return success;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=- InitGL -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool initGL(){
    GLenum error = GL_NO_ERROR;
    unsigned int i;
    
    // global GL settings go here
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    
    // LOAD SHADERS
    //////////////////
    
	// load regular 3D scene shader
	if( loadProgram( gProgramID, "vshader.txt", "fshader.txt", "" ) == false ){
		printf( "ERROR: Loading shader program failed!\n" );
		return false;
	}
	
	// Create a separate shader program for rendering a texture to the screen, for post-processing
	if( loadProgram( gScreenProgram, "vscreen.txt", "fscreen.txt", "" ) == false ){
		printf( "ERROR: Loading screen shader program failed!\n" );
		return false;
	}
	
	// Create a shader program for processing a Gaussian blur
	if( loadProgram( gBlurProgram, "vblur.txt", "fblur.txt", "" ) == false ){
		printf( "ERROR: Loading blur shader program failed!\n" );
		return false;
	}
	
	// Create a shader program for processing Bloom effect
	if( loadProgram( gBloomProgram, "vbloom.txt", "fbloom.txt", "" ) == false ){
		printf( "ERROR: Loading bloom shader program failed!\n" );
		return false;
	}
	
	// Create a shader program for shadow mapping
	if( loadProgram( gShadowProgram, "vshadow.txt", "fshadow.txt", "gshadow.txt" ) == false ){
		printf( "ERROR: Loading shadow shader program failed!\n" );
		return false;	
	}
	
	// Create a shader program for God Rays
	if( loadProgram( gRaysProgram, "vrays.txt", "frays.txt", "" ) == false ){
		printf( "ERROR: Loading god-rays shader program failed!\n" );
		return false;
	}
	
	
    
    // figure out screen dimensions
    unsigned int fbWidth= USE_LORES ? LORES_WIDTH : SCREEN_WIDTH;
    unsigned int fbHeight = USE_LORES ? LORES_HEIGHT : SCREEN_HEIGHT;
    
    // Set the rendering viewport according to the screen dimensions we wanna use
    glViewport( 0, 0, fbWidth, fbHeight );
    
    
    // FRAME BUFFER OBJECTS FOR SHADER PROGRAMS
    ////////////////////////////////////////////
    
    // create and bind a framebuffer object
    glGenFramebuffers( 1, &gFBO );
    glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
    
    // create two offscreen color buffers for bloom color effects
    glGenTextures(2, gColorBuffers );
	for ( i = 0; i < 2; i++ )
	{
	    glBindTexture( GL_TEXTURE_2D, gColorBuffers[i] );
	    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, fbWidth, fbHeight, 0, GL_RGB, GL_FLOAT, NULL );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	    // attach texture to framebuffer
	    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, gColorBuffers[i], 0 );
	}

    // Create a Renderbuffer to put depth and stencil info
    glGenRenderbuffers( 1, &gRBO );
    glBindRenderbuffer( GL_RENDERBUFFER, gRBO );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );
    // add renderbuffer to our framebuffer
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRBO );
    
    // Tell OpenGL the assignment of each color buffer element
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers( 2, attachments );
    
    // check for Framebuffer errors
    error = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if( error != GL_FRAMEBUFFER_COMPLETE ){
    	printf( "ERROR: " );
    	switch( error ){
    		case 36054:
    			printf( "Incomplete attachment, check if zero or undefined before attaching.\n" );
    			break;
    		case 36057:
    			printf( "Not all attachments have the same width/height.\n" );
    			break;
    		case 36055:
    			printf( "No images are attached to the framebuffer.\n" );
    			break;
    		case 36061:
    			printf( "The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.\n" );
    	}
    	return false;
    }
    
    // Create two framebuffer objects for Gaussian blue shader
    glGenFramebuffers( 2, gBlurFBOs );
    glGenTextures( 2, gBlurColorBuffers );
    for ( i = 0; i < 2; i++ ){
        glBindFramebuffer( GL_FRAMEBUFFER, gBlurFBOs[i] );
        glBindTexture( GL_TEXTURE_2D, gBlurColorBuffers[i] );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, fbWidth, fbHeight, 0, GL_RGB, GL_FLOAT, NULL );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBlurColorBuffers[i], 0 );
		// check if framebuffers are complete (no need for depth buffer)
        if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
            printf( "ERROR: Create blurring framebuffer failed!\n" );
    }
    
    // Lo-Res
	glGenFramebuffers( 1, &gLoresFBO );
    glGenTextures( 1, &gLoresColorBuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, gLoresFBO );
    glBindTexture( GL_TEXTURE_2D, gLoresColorBuffer );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, fbWidth, fbHeight, 0, GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gLoresColorBuffer, 0 );
	// check if framebuffers are complete (no need for depth buffer)
    if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
        printf( "ERROR: Create Lores framebuffer failed!\n" );
        
    // Shadow Mapping Framebuffers and depth surface
    glGenFramebuffers( 1, &gShadowFBO );
    glGenTextures( 1, &gShadowBuffer );
    glBindTexture( GL_TEXTURE_CUBE_MAP, gShadowBuffer );
    for( i = 0; i < 6; ++i )
    {
    	glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_RES, SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
    }
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
	
    glBindFramebuffer( GL_FRAMEBUFFER, gShadowFBO );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gShadowBuffer, 0 );
    glDrawBuffer( GL_NONE );  // Tell OpenGL we don't want to draw expensive color information,
    glReadBuffer( GL_NONE );  // all we want is depth info for shadow calculations.
    if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
        printf( "ERROR: Create shadow framebuffer failed!\n" );
    
    // God Rays
    glGenFramebuffers( 1, &gRaysFBO );
    glGenTextures( 1, &gRaysBuffer );
    glBindTexture( GL_TEXTURE_2D, gRaysBuffer );
    glBindFramebuffer( GL_FRAMEBUFFER, gRaysFBO );
    glBindTexture( GL_TEXTURE_2D, gRaysBuffer );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, fbWidth * GODRAYS_SCALE, fbHeight * GODRAYS_SCALE, 0, GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gRaysBuffer, 0 );
	// also check if framebuffers are complete (no need for depth buffer)
    if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
        printf( "ERROR: Create Lores framebuffer failed!\n" );

	// Bind the screen space, we're done creating off-screen buffers
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    
    
    
    // Create shader program parameters
    glUseProgram( gProgramID );
    glUniform1i( glGetUniformLocation( gProgramID, "diffuseTexture" ), 0 );
    glUniform1i( glGetUniformLocation( gProgramID, "shadowMap" ), 1 );
    
    glUseProgram( gBlurProgram );
    glUniform1i( glGetUniformLocation( gBlurProgram, "image" ), 0 );
    
    glUseProgram( gBloomProgram );
    glUniform1i( glGetUniformLocation( gBloomProgram, "scene" ), 0);
    glUniform1i( glGetUniformLocation( gBloomProgram, "bloomBlur" ), 1);
    
    glUseProgram( gShadowProgram );
    glUniform1i( glGetUniformLocation( gShadowProgram, "alphatex"), 0 );



	// GEOMETRY
	//////////////////
    
    // Cube
    float vertexData[] =
    {
        // positions                        // normals         // uv coords
        // north
	    -CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
	     CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
	    -CUBE_SIZE,  CUBE_SIZE, CUBE_SIZE,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
	     CUBE_SIZE,  CUBE_SIZE, CUBE_SIZE,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
	    
	    // south
	    -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
	     CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
	    -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
	     CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
	    
	    // east
	    CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
	    CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
	    CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
	    CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
	    
	    // west
	    -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
	    -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
	    -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
	    -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
	    
	    // top
	    -CUBE_SIZE, CUBE_SIZE,  CUBE_SIZE,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
	     CUBE_SIZE, CUBE_SIZE,  CUBE_SIZE,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
	    -CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
	     CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
	    
	    // bottom
	    -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
	     CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
	    -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
	     CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f
    };
    
    unsigned int indices[] = {
    	// north
		0, 1, 2,
		1, 3, 2,
    	
    	// south
    	6, 5, 4,
    	7, 5, 6,
    	
    	// east
    	10, 9, 8,
    	10, 11, 9,
    	
    	// west
    	12, 13, 14,
    	14, 13, 15,
    	
    	// top
    	16, 17, 18,
    	17, 19, 18,
    	
    	// bottom
    	22, 21, 20,
    	23, 21, 22
    };

    //Create a Vertex Buffer Object, and send our vertex data and specifications to OpenGL
    glGenVertexArrays( 1, &gVAO );
    glBindVertexArray( gVAO );
    
    glGenBuffers( 1, &gVBO );
    glGenBuffers( 1, &gEBO );
    
    // Vertex data
    glBindBuffer( GL_ARRAY_BUFFER, gVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertexData ), vertexData, GL_STATIC_DRAW );
    
	// index/element data
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );
    
    // Set vertex attributes for the "scene" vertex array
    // position attribute
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0 );
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)) );
	glEnableVertexAttribArray(1);
	// texture attribute
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)) );
	glEnableVertexAttribArray(2);
	
	
	
	//Vertex Buffer Object - Floor!
    float vertexFloor[] =
    {
        // positions         // normals         // uv coords
		-100.f, -FLOOR_HEIGHT,  100.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
	     100.f, -FLOOR_HEIGHT,  100.0f,	 0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
	    -100.f, -FLOOR_HEIGHT, -100.0f,	 0.0f, 1.0f, 0.0f,  0.0f, 10.0f,
	     100.f, -FLOOR_HEIGHT, -100.0f,	 0.0f, 1.0f, 0.0f,  10.0f, 10.0f  
    };
    
    unsigned int indicesFloor[] = {
		0, 1, 2,
		1, 3, 2
    };

    //Create a Vertex Buffer Object, and send our vertex data and specifications to OpenGL
    glGenVertexArrays( 1, &gVAOfloor );
    glBindVertexArray( gVAOfloor );
    
    glGenBuffers( 1, &gVBOfloor );
    glGenBuffers( 1, &gEBOfloor );
    
    // Vertex data
    glBindBuffer( GL_ARRAY_BUFFER, gVBOfloor );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertexFloor ), vertexFloor, GL_STATIC_DRAW );
    
	// index/element data
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOfloor );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indicesFloor ), indicesFloor, GL_STATIC_DRAW );
    
    // Set vertex attributes for the "scene" vertex array
    // position attribute
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0 );
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)) );
	glEnableVertexAttribArray(1);
	// texture attribute
	glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)) );
	glEnableVertexAttribArray(2);
    
    
    // Here we will create another Vertex Array Object containing a simple quad to use
    // when we draw a copy of an offscreen buffer to screen memory, used for post-processing effects.
    glGenVertexArrays( 1, &VAO_screen );
    glBindVertexArray( VAO_screen );
    
    float screen_verts[] = {
    	-1.f, -1.f, 0.0f,  0.0f, 0.0f,
	     1.f, -1.f, 0.0f,  1.0f, 0.0f,
	    -1.f,  1.f, 0.0f,  0.0f, 1.0f,
	     1.f,  1.f, 0.0f,  1.0f, 1.0f
	};
	
	unsigned int screen_inds[] = {
		0, 1, 2,
		2, 1, 3
	};
	
	glGenBuffers( 1, &VBO_screen );
    glGenBuffers( 1, &EBO_screen );
	
	glBindBuffer( GL_ARRAY_BUFFER, VBO_screen );
	glBufferData( GL_ARRAY_BUFFER, sizeof( screen_verts ), screen_verts, GL_STATIC_DRAW );
		
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, EBO_screen );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( screen_inds ), screen_inds, GL_STATIC_DRAW );
    
	// Set the vertex attributes for the "screen" vertex array
	// position attribute
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0 );
	glEnableVertexAttribArray(0);
	// texture attribute
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)) );
	glEnableVertexAttribArray(1);
	
	
	
	
	// MATRICES
	///////////////
	view = glm::translate( view, -viewPos ); // negative, the "world" is translated in the opposite direction of any "camera" movement
	proj = glm::perspective( glm::radians( 45.0f ), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 1.f, 1000.0f );
	
	
	
	
	// TEXTURES
	///////////////
	gTex = 		loadTexFromFile( "trans.png", 256, 256, true, !USE_LORES );
	gFloortex =	loadTexFromFile( "tile2.png", 512, 512, true, !USE_LORES );


	printf( "SUCCESS: OpenGL initialized...\n" );
    return true;
}






// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=- update -=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void update( float delta ){
	float theta = 0.5f;
	static float lightAngle = 0.0f;
	
	// rotate the model matrix (note: this applies a rotation each frame, causing the triangle to spin during the loop)
	model    = glm::rotate( model,    glm::radians( theta * delta ), glm::vec3( 1.0f, 1.0f, 1.0f ) );
	matFloor = glm::rotate( matFloor, glm::radians( ( theta * 0.075f ) * delta ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	
	if( MOVE_LIGHT )
	{
		lightAngle += 0.0075f * delta;
		if( lightAngle > 3.14159*2 ) lightAngle = 0;
		
		lightPos.x = cos( lightAngle ) * lightDistance;
		lightPos.z = sin( lightAngle ) * lightDistance;
	}
}

void renderCube(){
	// Note: Culling disabled temporarily to show backside(s) if there is transparent texture
	glDisable( GL_CULL_FACE );
	glBindVertexArray( gVAO );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, gTex );
	glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
	glEnable( GL_CULL_FACE );
}

void renderFloor(){
	glBindVertexArray( gVAOfloor );
	glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, gFloortex );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
}

void renderQuad(){
	glBindVertexArray( VAO_screen );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );	
}



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// -=-=-=-=- processShadows -=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void processShadows( float far_plane ) {
	float near_plane = 1.0f;
	
	// In this example we have a point light and shadows
	// are produced using a "perspective" transform
	glm::mat4 shadowProj = glm::perspective( glm::radians( 90.0f ), 1.0f, near_plane, far_plane );
	
	// We will need 6 elements arrayed inside a vector - one for each cube face
	std::vector<glm::mat4> shadowTransforms;
	
	// populate vector with the 6 different directions of the cube map
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3( 1.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, -1.0f,  0.0f ) ) );
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, -1.0f,  0.0f ) ) );
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3( 0.0f, 1.0f, 0.0f ), glm::vec3( 0.0f,  0.0f,  1.0f ) ) );
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3( 0.0f,-1.0f, 0.0f ), glm::vec3( 0.0f,  0.0f, -1.0f ) ) );
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3( 0.0f, 0.0f, 1.0f ), glm::vec3( 0.0f, -1.0f,  0.0f ) ) );
	shadowTransforms.push_back( shadowProj * glm::lookAt( lightPos, lightPos + glm::vec3( 0.0f, 0.0f,-1.0f ), glm::vec3( 0.0f, -1.0f,  0.0f ) ) );
	
	// Use the shadow rendering program
	glUseProgram( gShadowProgram );
	
	// adjust viewport before rendering
	glViewport( 0, 0, SHADOW_RES, SHADOW_RES );
	glBindFramebuffer( GL_FRAMEBUFFER, gShadowFBO );
	glClear( GL_DEPTH_BUFFER_BIT );
	
	for( int i = 0; i < 6; i++ )
		setMat4( gShadowProgram, "shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i] );
	
	glUniform1f( glGetUniformLocation( gShadowProgram, "far_plane" ), far_plane );
	glUniform3f( glGetUniformLocation( gShadowProgram, "lightPos" ), lightPos.x, lightPos.y, lightPos.z );
		
	// render floor
	shaderSendMatrix( glGetUniformLocation( gShadowProgram, "model" ), matFloor );
	renderFloor();
	
	// render cube - with transparency
	shaderSendMatrix( glGetUniformLocation( gShadowProgram, "model" ), model );
	renderCube();
	
	// reset viewport
	setViewport();
}



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=- render -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void render()
{
	// SHADOWS:
	// Set up a cubemap and render depth information
	float far_plane = 100.0f;
	processShadows( far_plane );
	
	
	// COLOR:
	// Render color information (including shadows) and pass only highlights
	// into secondary buffer, to be processed later
	glUseProgram( gProgramID );
	
	// Submit scene transform matrices
	glm::mat3 matNormal(1.0f); // declare a matrix specific to only modifying normals
	
	int viewLocation = glGetUniformLocation( gProgramID, "view" );
	shaderSendMatrix( viewLocation, view );
	
	int projLocation = glGetUniformLocation( gProgramID, "projection" );
	shaderSendMatrix( projLocation, proj );
	
	int modelLocation = glGetUniformLocation( gProgramID, "model" );
	int normalLocation = glGetUniformLocation( gProgramID, "normal_matrix" );
	
	// Calculate light's 2D position in screen space
	glm::mat4 transform = proj * view;
	glm::vec4 lightPosition2D( lightPos.x, lightPos.y, lightPos.z, 1.0f );
	lightPosition2D = transform * lightPosition2D;
	
	lightPosition2D.x /= lightPosition2D.w;
	lightPosition2D.y /= lightPosition2D.w;
	lightPosition2D.x += 1.0f;
	lightPosition2D.y += 1.0f;
	lightPosition2D.x *= SCREEN_WIDTH / 2;
	lightPosition2D.y *= SCREEN_HEIGHT / 2;	
	
	// Prepare scene lighting
	glUniform3f( glGetUniformLocation( gProgramID, "lightColor" ), 10.f, 9.f, 5.f );
	glUniform3f( glGetUniformLocation( gProgramID, "lightPos" ), lightPos.x, lightPos.y, lightPos.z );
	glUniform3f( glGetUniformLocation( gProgramID, "viewPos" ), viewPos.x, viewPos.y, viewPos.z );
	glUniform1f( glGetUniformLocation( gProgramID, "far_plane" ), far_plane );
	glUniform2f( glGetUniformLocation( gProgramID, "lightPos2D" ), lightPosition2D.x, lightPosition2D.y );
	
	glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// Texture0 - Regular color object texture (set in rendering functions)
	// Texture1 - cubemap depth info for shadows
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, gShadowBuffer );
	
    // DRAW THE FLOOR
    // Update the floor's transform matrix
    shaderSendMatrix( modelLocation, matFloor );
    
	// Calculate the normals for the floor
	matNormal = getNormalMatrix( matFloor );
	shaderSendMatrix( normalLocation, matNormal );
	
	// Render the floor's textured geometry
	if( DRAW_FLOOR ) renderFloor();
	
	// Draw a fullbright mini cube at the Light's position
	glm::mat4 lightTranslate = glm::translate( glm::mat4(1.0f), lightPos );
	glm::mat4 lightScale = glm::scale( glm::mat4( 1.0f ), glm::vec3( 0.1f, 0.1f, 0.1f ) );
	glm::mat4 light_matrix = lightTranslate * lightScale;
	
	shaderSendMatrix( modelLocation, light_matrix );
	glUniform1i( glGetUniformLocation( gProgramID, "fullbright" ), true );
	renderCube();
	glUniform1i( glGetUniformLocation( gProgramID, "fullbright" ), false );
	
	// Draw cube last, allows for transparency
	matNormal = getNormalMatrix( model );
	shaderSendMatrix( normalLocation, matNormal );
	shaderSendMatrix( modelLocation, model );
	if( DRAW_CUBE ) renderCube();
	
	
    // GAUSSIAN BLUR:
    // Blur the secondary highlight buffer
    bool horizontal = true, first_iteration = true;
    unsigned int amount = BlurAmount, index, inverse;
    glUseProgram( gBlurProgram );
    for (unsigned int i = 0; i < amount; i++)
    {
    	if( horizontal ){ index = 1; inverse = 0; }
    	else { index = 0; inverse = 1; }
    	
        glBindFramebuffer( GL_FRAMEBUFFER, gBlurFBOs[index] );
        glUniform1i( glGetUniformLocation( gBlurProgram, "horizontal" ), horizontal );
        glBindTexture( GL_TEXTURE_2D, first_iteration ? gColorBuffers[1] : gBlurColorBuffers[inverse] );  // bind texture of other framebuffer (or scene if first iteration)
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    
    
    // BLOOM:
    // Additive blending of main color buffer and blurred "bloom" buffer
	glUseProgram( gBloomProgram );
	
	// We must draw onto yet another offscreen buffer if we're in "Lo-Res" mode, otherwise draw right to the screen
	if( USE_LORES )
		glBindFramebuffer( GL_FRAMEBUFFER, gLoresFBO );	// Copy to Lores framebuffer, for later
	else
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );	// Draw directly to screen surface

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Pass along rendering parameters we can control programatically, using keyboard input
	glUniform1i( glGetUniformLocation( gBloomProgram, "bloom" ), BLOOM );
    glUniform1f( glGetUniformLocation( gBloomProgram, "exposure" ), EXPOSURE );
    
    // Texture0 = regular scene color information
	glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, gColorBuffers[0] );
    
    // Texture1 - Highlights-only that have been blurred, will be drawn using additive blending within shader
	glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, gBlurColorBuffers[!horizontal] );

    renderQuad();
    
    // If we are using Lo-Res mode, scale up the drawing viewport, and remove texture filtering (more pixels!!!)
    if( USE_LORES ){
    	glUseProgram( gScreenProgram );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, gLoresColorBuffer );
		glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
		renderQuad();
		glViewport( 0, 0, LORES_WIDTH, LORES_HEIGHT );
    }
}



// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=- main -=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
int main() {
	bool quit = false;
	SDL_Event e;
	
	unsigned int lastTick, currentTick=SDL_GetTicks();
	float delta;

	// Init SDL
	if( init() ){
		
		// Use key input for our program
		SDL_StartTextInput();
		
		// Bind our "scene" shader
		glUseProgram( gProgramID );
		
	    ////////////////////////////
	    //////// Main Loop /////////
	    ////////////////////////////
	    while( !quit ){
	    	
			// calculate the time difference since the last loop
			lastTick = currentTick;
	    	currentTick = SDL_GetTicks();
	    	
	    	delta = (float)(currentTick - lastTick) / 16.666666666667f;	// SDL is locked in at 60 fps... usually...
	    	
	    	
	        //Handle events on queue
	        while( SDL_PollEvent( &e ) != 0 )
	        {
	        	//User requests quit
	            if( e.type == SDL_QUIT ){
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
	        update( delta );
	        
	        // render scene
	        render();
	        
	        // draw to screen
	        SDL_GL_SwapWindow( gWindow );
	        
		} printf( "ATTEMPT: Exiting loop, cleaning up...\n" );
	}
	
	SDL_StopTextInput();

	// exit the shader program
	glUseProgram( 0 );
	
    // clean up
    close();
    
    printf( "Quitting...\n" );
	return 0;	
}

void close(){
	glDeleteBuffers( 1, &VBO_screen );
	glDeleteBuffers( 1, &EBO_screen );
	glDeleteBuffers( 1, &gVBO );
	glDeleteBuffers( 1, &gEBO );
	glDeleteBuffers( 1, &gVBOfloor );
	glDeleteBuffers( 1, &gEBOfloor );

	glDeleteVertexArrays( 1, &gVAOfloor );
	glDeleteVertexArrays( 1, &gVAO );
	glDeleteVertexArrays( 1, &VAO_screen );
	
	glDeleteFramebuffers( 1, &gRaysFBO );
	glDeleteFramebuffers( 1, &gShadowFBO );
	glDeleteFramebuffers( 1, &gLoresFBO );
	glDeleteFramebuffers( 1, &gFBO );
	glDeleteFramebuffers( 2, gBlurFBOs );
	glDeleteRenderbuffers( 1, &gRBO );
	
	glDeleteTextures( 1, &gRaysBuffer );
	glDeleteTextures( 1, &gShadowBuffer );
	glDeleteTextures( 1, &gLoresColorBuffer );
	glDeleteTextures( 1, &gFloortex );
	glDeleteTextures( 1, &gTex );
	glDeleteTextures( 2, gColorBuffers );
	glDeleteTextures( 2, gBlurColorBuffers );
	
	glDeleteProgram( gRaysProgram );
	glDeleteProgram( gScreenProgram );
	glDeleteProgram( gProgramID );
	glDeleteProgram( gBlurProgram );
	glDeleteProgram( gBloomProgram );
	glDeleteProgram( gShadowProgram );

	SDL_DestroyWindow( gWindow );
	SDL_GL_DeleteContext( gContext );
	IMG_Quit();
	SDL_Quit();	
}

void handleKeys( unsigned char key, int x, int y ){
    // lower blur amount
    if( key == 'q' )
    	BlurAmount--;

    if( key == 'w' )
    	BlurAmount++;
    	
    if( key == 'a' )
    	EXPOSURE -= 0.05f;
    	
    if( key == 's' )
    	EXPOSURE += 0.05f;
    	
    if( key == 'z' )
    	BLOOM = !BLOOM;
    	
    if( key == 'x' )
    	MOVE_LIGHT = !MOVE_LIGHT;
    	
    if( key == 'c' )
    	DRAW_CUBE = !DRAW_CUBE;
    	
    if( key == 'f' )
    	DRAW_FLOOR = !DRAW_FLOOR;
    	
    if( key == 'l' )
    	lightDistance += 1.0f;
    	
    if( key == 'k' )
    	lightDistance -= 1.0f;
    	
    if( key == 'o' )
    	lightPos.y += 1.0f;
    	
    if( key == ',' )
    	lightPos.y -= 1.0f;
    	
    if( key == '1' )
    	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    	
    if( key == '2' )
    	glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    	
    if( key == '3' )
    	glClearColor( 0.1f, 0.2f, 0.3f, 1.0f );
    	
    if( BlurAmount < 2 ) BlurAmount = 2;
    if( EXPOSURE < 0.0f ) EXPOSURE = 0.0f;
    if( lightDistance < 1.0f ) lightDistance = 1.0f;
}

// function for 4x4 matrix
void shaderSendMatrix( unsigned int location, glm::mat4 &matrix ){
	glUniformMatrix4fv( location, 1, GL_FALSE, glm::value_ptr( matrix ) );
}

// function for 3x3 matrix
void shaderSendMatrix( unsigned int location, glm::mat3 &matrix ){
	glUniformMatrix3fv( location, 1, GL_FALSE, &matrix[0][0] );
}

void setMat4(unsigned int &ID, const std::string &name, const glm::mat4 &mat)
{
	GLuint loc = glGetUniformLocation( ID, name.c_str() );
    glUniformMatrix4fv( loc, 1, GL_FALSE, &mat[0][0] );
}

void setViewport(){
	if( USE_LORES )
		glViewport( 0, 0, LORES_WIDTH, LORES_HEIGHT );
	else
		glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
}

glm::mat3 getNormalMatrix( glm::mat4 inMatrix ){
	return glm::transpose( glm::inverse( inMatrix ) );
}


