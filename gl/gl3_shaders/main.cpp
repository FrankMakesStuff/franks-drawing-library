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
const int LORES_WIDTH		= 240;
const int LORES_HEIGHT		= 150;

SDL_Window* gWindow = NULL;		// The window we'll be rendering to
SDL_GLContext gContext;			// OpenGL context
bool gRenderQuad = true;		// Render flag

// shader globals
GLuint gProgramID = 0;

GLint gVertexPos2DLocation = -1;
GLint gColorLocation = -1;

GLuint gVBO = 0;
GLuint gEBO = 0;
GLuint gVAO = 0;
GLuint gFBO = 0;
GLuint gRBO = 0;
GLuint gOffscreenTex = 0;

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
    
    // enable z-buffer
    // glEnable( GL_DEPTH_TEST );
    
	// load default program
	if( loadProgram( gProgramID ) == false ){
		printf( "ERROR: Loading shader program failed!\n" );
		success = false;
	}
	else
    {
        // Lets start with initting basic GL stuff
        glClearColor( 0.1f, 0.2f, 0.3f, 1.0f );
        
        // LO-RES STUFF!!!!!!!
        
    	// create and bind a framebuffer object
        glGenFramebuffers( 1, &gFBO );
        glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
        
        // create and bind an offscreen texture buffer
        glGenTextures( 1, &gOffscreenTex );
        glBindTexture( GL_TEXTURE_2D, gOffscreenTex );
        
        // create an offscreen color plane and define its parameters
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, LORES_WIDTH, LORES_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glBindTexture( GL_TEXTURE_2D, 0 );
        
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gOffscreenTex, 0 );
        
        // for depth and stencil capabilities, create a renderbuffer
        glGenRenderbuffers( 1, &gRBO );
        glBindRenderbuffer( GL_RENDERBUFFER, gRBO );
        glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, LORES_WIDTH, LORES_HEIGHT );
        glBindRenderbuffer( GL_RENDERBUFFER, 0 );
        
        // add renderbuffer to our framebuffer
        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRBO );
        
        // 3D viewport must be set to lo-res dimensions
        glViewport( 0, 0, LORES_WIDTH, LORES_HEIGHT );
        
        // Switch back to the window-system-provided framebuffer
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        
        // check for errors
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
        }

        //VBO data
        float vertexData[] =
        {
            // positions         // colors          // uv coords
		    -3.f, -3.f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
		    3.f, -3.f, 0.0f,   0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
		    -3.f, 3.f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
		    3.f, 3.f, 0.0f,    1.0f, 1.0f, 0.0f,  1.0f, 0.0f
        };
        
        unsigned int indices[] = {
        	0, 1, 2,
        	1, 2, 3
        };

        //Create Vertex Buffer Object, and send our vertex data and specifications to OpenGL
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
		view = glm::translate( view, glm::vec3( 0.0f, 0.0f, -10.0f ) );
		proj = glm::perspective( glm::radians( 45.0f ), (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0.1f, 100.0f );
	}

	if( success ) printf( "SUCCESS: OpenGL initialized...\n" );
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
    
    //Render quad
    if( gRenderQuad )
    {
    	// bind our lo-res framebuffer, clear it, and draw onto it
    	glBindFramebuffer( GL_FRAMEBUFFER, gFBO );
    	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    	glBindVertexArray( gVAO );
        glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );
    }
    
    // update lo-res framebuffer
    glBindFramebuffer( GL_READ_FRAMEBUFFER, gFBO ); // our lo-res framebuffer we created
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );   // value of '0' is reserved for the main gl framebuffer
    glBlitFramebuffer( 0, 0, LORES_WIDTH, LORES_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST );
}

void close(){
	glDeleteFramebuffers( 1, &gFBO );
	glDeleteRenderbuffers( 1, &gRBO );
	glDeleteVertexArrays( 1, &gVAO );
	glDeleteBuffers( 1, &gVBO );
	glDeleteBuffers( 1, &gEBO );
	
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
	if( init() ){
		// bind our shader program (we only have one)
		glUseProgram( gProgramID );
		std::string imageName = "mustard.png";
		std::string imagePath = SDL_GetBasePath() + imageName;
		printf("ATTEMPT: Loading textures...\n" );
		SDL_Surface *tex = IMG_Load( imagePath.c_str() );
		if ( tex == nullptr ){
			close();
			printf( "ERROR: Could not load texture - %s\n", SDL_GetError() );
			return 1;
		} else {
			printf( "SUCCESS: Loaded texture: %s\n", imageName.c_str() );
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
		} printf( "ATTEMPT: Exiting loop, cleaning up...\n" );
	}

	// exit the shader program
	glUseProgram( 0 );
	
    // clean up
    close();
    
    printf( "Quitting...\n" );
	return 0;	
}


