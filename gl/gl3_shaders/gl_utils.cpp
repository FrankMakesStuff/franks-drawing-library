// these preprocessor instructions are needed by SDL2 and glew32 to compile successfully
#define SDL_MAIN_HANDLED
#define GLEW_STATIC

#include "gl_utils.h"

const char *FRAG_SHADER_SOURCE = "fshader.txt";
const char *VERT_SHADER_SOURCE = "vshader.txt";

////////////////////////////////
/////// GL_UTILS.CPP ///////////
////////////////////////////////

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void printProgramLog( GLuint program )
{
    //Make sure name is shader
    if( glIsProgram( program ) )
    {
        //Program log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        char* infoLog = new char[ maxLength ];
        
        //Get info log
        glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 )
        {
            //Print Log
            printf( "%s\n", infoLog );
        }
        
        //Deallocate string
        delete[] infoLog;
    }
    else
    {
        printf( "ERROR: Name %d is not a program\n", program );
    }
}

void printShaderLog( GLuint shader )
{
    //Make sure name is shader
    if( glIsShader( shader ) )
    {
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        char* infoLog = new char[ maxLength ];
        
        //Get info log
        glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 )
        {
            //Print Log
            printf( "%s\n", infoLog );
        }

        //Deallocate string
        delete[] infoLog;
    }
    else
    {
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
	else
		sType = "frag";
	
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

bool loadProgram(GLuint &id){
		// create a program
		id = glCreateProgram();
		printf( "SUCCESS: Shader program created...\n", (unsigned int)id );
		
		// load vertex shader from file
		GLuint vertexShader = loadShaderFromFile( VERT_SHADER_SOURCE, GL_VERTEX_SHADER );
		if( vertexShader == 0 ){
			glDeleteProgram( id );
			id = 0;
			return false;	
		}
		
		// attach vertex shader to program
		glAttachShader( id, vertexShader );
		
		// create fragment shader
		GLuint fragmentShader = loadShaderFromFile( FRAG_SHADER_SOURCE, GL_FRAGMENT_SHADER );
		if( fragmentShader == 0 ){
			glDeleteShader( vertexShader );
			glDeleteProgram( id );
			id = 0;
			return false;	
		}
		
		// attach fragment shader to program
		glAttachShader( id, fragmentShader );
		
		printf( "ATTEMPT: attach GL program...\n" );
		// link program
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
	        glDeleteProgram( id );
	        id = 0;
	        return false;
	    } printf( "SUCCESS: Shaders linked to to GL program...\n" );

    //Clean up excess shader references
    glDeleteShader( vertexShader );
    glDeleteShader( fragmentShader );

    return true;
}
















