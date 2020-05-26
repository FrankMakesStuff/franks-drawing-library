// these preprocessor instructions are needed by SDL2 and glew32 to compile successfully
#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include "gl_utils.h"

////////////////////////////////
/////// GL_UTILS.CPP ///////////
////////////////////////////////

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printProgramLog( GLuint program ){
    //Make sure name is shader
    if( glIsProgram( program )){
        //Program log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        char* infoLog = new char[ maxLength ];
        
        //Get info log
        glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 ){
            //Print Log
            printf( "%s\n", infoLog );
        }
        
        //Deallocate string
        delete[] infoLog;
    }
    else{
        printf( "ERROR: Name %d is not a program\n", program );
    }
}

void printShaderLog( GLuint shader ){
    //Make sure name is shader
    if( glIsShader( shader )){
    	
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        char* infoLog = new char[ maxLength ];
        
        //Get info log
        glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 ){
            //Print Log
            printf( "%s\n", infoLog );
        }

        //Deallocate string
        delete[] infoLog;
    }
    else{
        printf( "ERROR: Name %d is not a shader\n", shader );
    }
}

GLuint loadShaderFromFile( std::string path, GLenum shaderType ){
	GLuint shaderID = 0;
	std::string shaderString;
	std::ifstream sourceFile( path.c_str() );
	
	std::string sType;
	if( shaderType == GL_VERTEX_SHADER )
		sType = "vert";
	
	if( shaderType == GL_FRAGMENT_SHADER )
		sType = "frag";
		
	if( shaderType == GL_GEOMETRY_SHADER )
		sType = "geom";
	
	if( sourceFile ){
		// read the source file as one whole string
		printf( "ATTEMPT: Reading shader source: %s\n", path.c_str() );
		shaderString.assign( std::istreambuf_iterator<char>( sourceFile ), std::istreambuf_iterator<char>() );
		
		// create a shader ID
		shaderID = glCreateShader( shaderType );
		
		// begin to install the source code
		
		//printf( "Attempting source: %s\n", shaderString.c_str() );
		const GLchar *shaderSource = shaderString.c_str();
		glShaderSource( shaderID, 1, (const GLchar**)&shaderSource, NULL );
		
		// compile the source code
		glCompileShader( shaderID );
		
		// error check
		GLint shaderCompiled = GL_FALSE;
		glGetShaderiv( shaderID, GL_COMPILE_STATUS, &shaderCompiled );
		if( shaderCompiled != GL_TRUE ){
			printf( "ERROR: Unable to compile shader %d\nSource: %s\n", shaderID, path.c_str() );
			printShaderLog( shaderID );
			glDeleteShader( shaderID );
			shaderID = 0;
		}
	} else {
		printf( "ERROR: Unable to open file: %s\n", path.c_str() );
	}
	
	printf( "SUCCESS: Loaded %s shader.\n", sType.c_str() );
	return shaderID;
}

void setColor( GLint &location, GLfloat r, GLfloat g, GLfloat b ){
	glUniform4f( location, r, g, b, 1.0f );	
}

bool loadProgram(GLuint &id, std::string vertSource, std::string fragSource, std::string geoSource ){
		// create a program
		id = glCreateProgram();
		printf( "SUCCESS: Shader program created...\n", (unsigned int)id );
		
		// load vertex shader from file
		GLuint vertexShader = loadShaderFromFile( vertSource, GL_VERTEX_SHADER );
		if( vertexShader == 0 ){
			glDeleteProgram( id );
			id = 0;
			return false;	
		}
		
		// attach vertex shader to program
		glAttachShader( id, vertexShader );
		
		// create fragment shader
		GLuint fragmentShader = loadShaderFromFile( fragSource, GL_FRAGMENT_SHADER );
		if( fragmentShader == 0 ){
			glDeleteShader( vertexShader );
			glDeleteProgram( id );
			id = 0;
			return false;	
		}
		
		// attach fragment shader to program
		glAttachShader( id, fragmentShader );
		
		// load geometry shader from file
		GLuint geometryShader;
		if( !geoSource.empty() ){
			geometryShader = loadShaderFromFile( geoSource, GL_GEOMETRY_SHADER );
			if( geometryShader == 0 ){
				glDeleteShader( fragmentShader );
				glDeleteProgram( id );
				id = 0;
				return false;
			}
			
			// Attach geometry shader to program
			glAttachShader( id, geometryShader );
		}
		
		// Final link shader program to GL
		printf( "ATTEMPT: Link GL program...\n" );
		glLinkProgram( id );
		
		// error check
		GLint programSuccess = GL_TRUE;
		glGetProgramiv( id, GL_LINK_STATUS, &programSuccess );
		if( programSuccess != GL_TRUE )
	    {
	        printf( "ERROR: Failed to link program %d!\n", id );
	        printProgramLog( id );
	        glDeleteShader( vertexShader );
	        glDeleteShader( fragmentShader );
	        glDeleteShader( geometryShader );
	        glDeleteProgram( id );
	        id = 0;
	        return false;
	    } 
		
	printf( "SUCCESS: Shader program %d created...\n", id );

    //Clean up excess shader references
    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );
    glDeleteShader( geometryShader );

    return true;
}

GLuint loadTexFromFile( const char *filename, unsigned int width, unsigned int height, bool gammaCorrection=false, bool filtering=true ){
	GLuint tempID;
	
	// set our filename from the given parameter, and concat it at the end of the application path on disk
	std::string imageName = filename;
	std::string imagePath = SDL_GetBasePath() + imageName;
	
	// attempt to load the image file into an SDL surface using the SDL Image helper library
	printf("ATTEMPT: Loading texture: %s\n", imagePath.c_str() );
	SDL_Surface *tex = IMG_Load( imagePath.c_str() );
	
	// Return a fail if there was a problem
	if ( tex == nullptr ){
		printf( "ERROR: Could not load texture - %s\n", SDL_GetError() );
		return 0;
	} 
	else {
		printf( "SUCCESS: Loaded texture: %s\n", imageName.c_str() );
		
		glGenTextures( 1, &tempID );
		glBindTexture( GL_TEXTURE_2D, tempID );
		
		int mode = GL_RGBA;
		int internalformat;
		if( gammaCorrection)
			internalformat = GL_SRGB_ALPHA;
		else
			internalformat = GL_RGBA;
		
		// DEBUG Texture, just in case of file errors, etc...
		float pixels[] = {
			1.0f, 1.0f, 1.0f,	0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f,	1.0f, 1.0f, 1.0f
		};
		
		// test/checkerboard pixels
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels );
		
		// Actual texture loaded from surface/file
		glTexImage2D( GL_TEXTURE_2D, 0, internalformat, width, height, 0, mode, GL_UNSIGNED_BYTE, tex->pixels);
		// filtering
		if( filtering ){
			glGenerateMipmap( GL_TEXTURE_2D );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		} else {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		}
		
		GLenum err = GL_NO_ERROR;
		err = glGetError();
		if( err != GL_NO_ERROR ){
			printf( "ERROR: GL could not create texture - %s\n", gluErrorString( err ) );
			return 0;
		}
		
		if( tex ) SDL_FreeSurface( tex );
	}

	return tempID;
}
















