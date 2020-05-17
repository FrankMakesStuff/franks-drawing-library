// CONSOLE/LOG HANDLER
#include "frank_console.h"

HANDLE hConsole = NULL;
int logBGcolor = CONSOLE_BLACK;
int logFGcolor = CONSOLE_WHITE;

bool initConsole(){
	// attempt getting the handle ref to the standard console output
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	if( hConsole == NULL ) return false;
		
	logBGcolor = CONSOLE_BLACK;
	logFGcolor = CONSOLE_WHITE;
	
	return true;
}

void setConsoleFG( console_colors_t color ){
	logFGcolor = (int)color;
	return;
}

void setConsoleBG( console_colors_t color ){
	logBGcolor = (int)color;
	return;
}

void setConsoleColors( console_colors_t bg, console_colors_t fg ){
	setConsoleBG( bg );
	setConsoleFG( fg );	
}

unsigned int getConsoleColorIndex(){
	return ( logBGcolor + logFGcolor ) * 16;
}	

int consolePrint( const char *str ){
	if( hConsole == NULL ) return 1;
	
	SetConsoleTextAttribute( hConsole, getConsoleColorIndex() );
	std::cout << str;
	
	return 0;
}

int consoleLog( console_msgtype_t type, const char msg[] ){
	switch( type ){
		case CONSOLE_TEXT:		setConsoleColors( CONSOLE_BLACK, CONSOLE_WHITE );	break;
		case CONSOLE_ERROR:		setConsoleColors( CONSOLE_BLACK, CONSOLE_RED );		consolePrint( "ERROR: " );		break;
		case CONSOLE_WARNING:	setConsoleColors( CONSOLE_BLACK, CONSOLE_BROWN );	consolePrint( "WARNING: " );	break;
		case CONSOLE_SUCCESS:	setConsoleColors( CONSOLE_BLACK, CONSOLE_GREEN );	consolePrint( "SUCCESS: " );	break;
		case CONSOLE_VERBOSE:	setConsoleColors( CONSOLE_BLACK, CONSOLE_PURPLE );	consolePrint( "VERBOSE: " );	break;
	}
	
	setConsoleColors( CONSOLE_BLACK, CONSOLE_WHITE );
	consolePrint( msg );
	std::cout << std::endl;
}


	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
