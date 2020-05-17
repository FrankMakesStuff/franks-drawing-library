#pragma once
// CONSOLE HANDLER HEADER
#include <iostream>
#include <windows.h>

typedef enum {
	CONSOLE_BLACK,
	CONSOLE_BLUE,
	CONSOLE_GREEN,
	CONSOLE_CYAN,
	CONSOLE_RED,
	CONSOLE_PURPLE,
	CONSOLE_BROWN,
	CONSOLE_GREY_LIGHT,
	CONSOLE_GREY_DARK,
	CONSOLE_BLUE_LIGHT,
	CONSOLE_GREEN_LIGHT,
	CONSOLE_CYAN_LIGHT,
	CONSOLE_PINK,
	CONSOLE_MAGENTA,
	CONSOLE_YELLOW,
	CONSOLE_WHITE } console_colors_t ;
	
typedef enum {
	CONSOLE_TEXT,
	CONSOLE_ERROR,
	CONSOLE_WARNING,
	CONSOLE_SUCCESS,
	CONSOLE_VERBOSE } console_msgtype_t ;
	
bool initConsole();
void setConsoleFG( console_colors_t color );
void setConsoleBG( console_colors_t color );
void setConsoleColors( console_colors_t bg, console_colors_t fg );
unsigned int getConsoleColorIndex();
int consolePrint( const char *str );
int consoleLog( console_msgtype_t type, const char msg[] );
