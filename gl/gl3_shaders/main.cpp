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
const char *FRAG_SHADER_SOURCE = "fshader.txt";
const char *VERT_SHADER_SOURCE = "vshader.txt";
const char *VERT_SCREEN_SOURCE = "vscreen.txt";
const char *FRAG_SCREEN_SOURCE = "fscreen.txt";

const bool USE_LORES = false;

const float CUBE_SIZE = 8.0f;
const float FLOOR_SIZE = 100.0f;
const float FLOOR_HEIGHT = 20.0f;

bool DRAW_CUBE = true;
bool DRAW_FLOOR = true;
bool BLOOM = true;
float EXPOSURE = 1.0f;
unsigned int BlurAmount = 10;
float lightDistance = 20.0f;

SDL_Window* gWindow = NULL;		// The window we'll be rendering to
SDL_GLContext gContext;			// OpenGL context

// shader programs
GLuint gProgramID = 0;
GLuint gScreenProgram = 0;
GLuint gBlurProgram = 0;
GLuint gBloomProgram = 0;

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

// LIGHT POSITION
glm::vec3 lightPos( -1.f, 3.f, 4.f );

// offscreen rendering objects
GLuint gFBO = 0; 				// main offscreen FBO
GLuint gColorBuffers[2];    	// 0=normal scene, 1=bright colors only
GLuint gRBO = 0;

// Gaussian blur
GLuint gBlurFBOs[2];			// offscreen Gaussian blur FBOs
GLuint gBlurColorBuffers[2];

// Low-Resolution low-color effect
GLuint gLoresFBO = 0;
GLuint gLoresColorBuffer = 0;

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

                //////////////////////
                /// INIT OPENGL //////
                //////////////////////
                if( !initGL() ){
                    printf( "ERROR: Unable to initialize OpenGL!\n" );
                    success = false;
                }
            }
        }
    }
    
    // initialize SDL image helper library
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
    bool success = true;
    GLenum error = GL_NO_ERROR;
    unsigned int i;
    
    // global GL settings go here
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_TEXTURE_2D );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    
	// load default program
	if( loadProgram( gProgramID, VERT_SHADER_SOURCE, FRAG_SHADER_SOURCE ) == false ){
		printf( "ERROR: Loading shader program failed!\n" );
		success = false;
	}
	else
    {
    	// Create a separate shader program for rendering a texture to the screen, for post-processing
    	if( loadProgram( gScreenProgram, VERT_SCREEN_SOURCE, FRAG_SCREEN_SOURCE ) == false ){
    		printf( "ERROR: Loading screen shader program failed!\n" );
    		success = false;
    		return success;
    	}
    	
    	// Create a shader program for processing a Gaussian blur
    	if( loadProgram( gBlurProgram, "vblur.txt", "fblur.txt" ) == false ){
    		printf( "ERROR: Loading blur shader program failed!\n" );
    		success = false;
    		return success;
    	}
    	
    	// Create a shader program for processing Bloom effect
    	if( loadProgram( gBloomProgram, "vbloom.txt", "fbloom.txt" ) == false ){
    		printf( "ERROR: Loading bloom shader program failed!\n" );
    		success = false;
    		return success;
    	}
        
        // figure out screen dimensions
        unsigned int fbWidth= USE_LORES ? LORES_WIDTH : SCREEN_WIDTH;
        unsigned int fbHeight = USE_LORES ? LORES_HEIGHT : SCREEN_HEIGHT;
        
        // Set the rendering viewport according to the screen dimensions we wanna use
        glViewport( 0, 0, fbWidth, fbHeight );
        
        // create and bind a framebuffer object
        glGenFramebuffers( 1, &gFBO );
        glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
        
        // create TWO offscreen color buffers for bloom color effects
        glGenTextures(2, gColorBuffers );
		for ( i = 0; i < 2; i++ )
		{
		    glBindTexture( GL_TEXTURE_2D, gColorBuffers[i] );
		    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, fbWidth, fbHeight, 0, GL_RGB, GL_FLOAT, NULL );
		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
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
        	success = false;
        	return success;
        }
        
        // Switch back to the window-system-provided framebuffer
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        
        // Create more Framebuffer objects for blurring shader
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
	        
			// also check if framebuffers are complete (no need for depth buffer)
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
	    
		// also check if framebuffers are complete (no need for depth buffer)
	    if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	        printf( "ERROR: Create Lores framebuffer failed!\n" );
	    
	    // Create shader program parameters
	    glUseProgram( gProgramID );
	    glUniform1i( glGetUniformLocation( gProgramID, "diffuseTexture" ), 0 );
	    
	    glUseProgram( gBlurProgram );
	    glUniform1i( glGetUniformLocation( gBlurProgram, "image" ), 0 );
	    
	    glUseProgram( gBloomProgram );
	    glUniform1i( glGetUniformLocation( gBloomProgram, "scene" ), 0);
	    glUniform1i( glGetUniformLocation( gBloomProgram, "bloomBlur" ), 1);

        //Vertex Buffer Object - cube!
        float vertexData[] =
        {
            // positions         // normals          // uv coords
            
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
			2, 1, 0,
			3, 2, 1,
        	
        	// south
        	4, 5, 6,
        	5, 6, 7,
        	
        	// east
        	8, 9, 10,
        	9, 10, 11,
        	
        	// west
        	14, 13, 12,
        	15, 14, 13,
        	
        	// top
        	16, 17, 18,
        	17, 18, 19,
        	
        	// bottom
        	22, 21, 20,
        	23, 22, 21
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
			-100.f, -10.f,  100.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
		     100.f, -10.f,  100.0f,	 0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
		    -100.f, -10.f, -100.0f,	 0.0f, 1.0f, 0.0f,  0.0f, 10.0f,
		     100.f, -10.f, -100.0f,	 0.0f, 1.0f, 0.0f,  10.0f, 10.0f  
        };
        
        unsigned int indicesFloor[] = {
    		0, 1, 2,
    		1, 2, 3
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
			1, 2, 3
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
		
		// initialize matrices
		//view = glm::rotate( view, glm::radians( 20.f ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
		view = glm::translate( view, glm::vec3( 0.f, 0.f, -50.f ) );
		proj = glm::perspective( glm::radians( 45.0f ), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 1.f, 1000.0f );
		
		// Load texture
		gTex = 		loadTexFromFile( "bright.png", 256, 256, true, !USE_LORES );
		gFloortex =	loadTexFromFile( "tile.png", 512, 512, true, !USE_LORES );
	}

	if( success ) printf( "SUCCESS: OpenGL initialized...\n" );
    return success;
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

void update(){
	float theta = 0.5f;
	static float lightAngle = 0.0f;
	
	// rotate the model matrix (note: this applies a rotation each frame, causing the triangle to spin during the loop)
	model    = glm::rotate( model,    glm::radians( theta ), glm::vec3( 1.0f, 1.0f, 1.0f ) );
	matFloor = glm::rotate( matFloor, glm::radians( theta ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	
	lightAngle += 0.03f;
	if( lightAngle > 3.14159*2 ) lightAngle = 0;
	
	lightPos.x = cos( lightAngle ) * lightDistance;
	lightPos.z = sin( lightAngle ) * lightDistance;
	
	// Set scene shader, and deliver matrix transforms
	glUseProgram( gProgramID );
	int modelLoc = glGetUniformLocation( gProgramID, "model" );
	glUniformMatrix4fv( modelLoc, 1, GL_FALSE, glm::value_ptr( model ) );	
	int viewLoc = glGetUniformLocation( gProgramID, "view" );
	glUniformMatrix4fv( viewLoc, 1, GL_FALSE, glm::value_ptr( view ) );
	int projLoc = glGetUniformLocation( gProgramID, "projection" );
	glUniformMatrix4fv( projLoc, 1, GL_FALSE, glm::value_ptr( proj ) );
}

void renderQuad(){
	glBindVertexArray( VAO_screen );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );	
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=- render -=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void render()
{
	
	// Prepare scene lighting
	glUniform3f(glGetUniformLocation( gProgramID, "objectColor" ), 1.f, 1.f, 1.f );
	glUniform3f(glGetUniformLocation( gProgramID, "lightColor" ), 10.f, 9.f, 5.f );
	glUniform3f(glGetUniformLocation( gProgramID, "lightPos" ), lightPos.x, lightPos.y, lightPos.z );
	
	// Render #1 - Draw illuminated 3D scenery into two color buffers - regular color and scene highlights/bright only
	glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// Prepare to draw spinning cube
	glBindVertexArray( gVAO );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, gTex );
	
	// Calculate new normals for all world matrix transformations 
	glm::mat3 matNormal = glm::transpose( glm::inverse( model ) );
	glUniformMatrix3fv( glGetUniformLocation( gProgramID, "normal_matrix" ), 1, GL_FALSE, &matNormal[0][0] );
    
	// Draw cube vertex array
	if( DRAW_CUBE )
		glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );
    
    // Next, we draw the floor
    // Apply the floor's unique matrix transformation
    glUniformMatrix4fv( glGetUniformLocation( gProgramID, "model" ), 1, GL_FALSE, glm::value_ptr( matFloor ) );
    
	// Calculate new normals for the floor
	matNormal = glm::transpose( glm::inverse( matFloor ) );
	glUniformMatrix3fv( glGetUniformLocation( gProgramID, "normal_matrix" ), 1, GL_FALSE, &matNormal[0][0] );
    
	// Draw the floor
	glBindVertexArray( gVAOfloor );
    glBindTexture( GL_TEXTURE_2D, gFloortex );
	if( DRAW_FLOOR )
		glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
    
    // Render #2 - Draw scene onto a quad, using the Gaussian blur shader
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
    
    // Render #3 - Bloom effect - Blend color buffer blurred, highlight-only buffer using additive blending
	glUseProgram( gBloomProgram );
	
	// This is the final render if we are not using Lores mode (where we need to do a bit more processing)
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
	        update();
	        
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
	
	glDeleteFramebuffers( 1, &gLoresFBO );
	glDeleteFramebuffers( 1, &gFBO );
	glDeleteFramebuffers( 2, gBlurFBOs );
	glDeleteRenderbuffers( 1, &gRBO );
	
	glDeleteTextures( 1, &gLoresColorBuffer );
	glDeleteTextures( 1, &gFloortex );
	glDeleteTextures( 1, &gTex );
	glDeleteTextures( 2, gColorBuffers );
	glDeleteTextures( 2, gBlurColorBuffers );
	
	glDeleteProgram( gScreenProgram );
	glDeleteProgram( gProgramID );
	glDeleteProgram( gBlurProgram );
	glDeleteProgram( gBloomProgram );

	SDL_DestroyWindow( gWindow );
	SDL_GL_DeleteContext( gContext );
	IMG_Quit();
	SDL_Quit();	
}


