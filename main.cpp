#define SDL_MAIN_HANDLED

#include <iostream>
#include <string>
#include "SDL2\SDL.h"
#include "SDL2\SDL_image.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int WINDOW_LEFT = 250;
const int WINDOW_TOP = 100;
const char *WINDOW_TITLE = "Frank's Demo";


/**
-=-=-=-=-=-=-=- LogError -=-=-=-=-=-
*/
void logError( std::ostream &os, const std::string &msg){
	os << msg << " error: " << SDL_GetError() << std::endl;	
}
void logVerbose( std::ostream &os, const std::string &msg ){
	os << msg << std::endl;	
}

/**
-=-=-=-=-=-=- LoadTexture -=-=-=-=-=-
*/
SDL_Texture* loadTexture(const std::string &file, SDL_Renderer *ren){
	SDL_Texture *texture = IMG_LoadTexture(ren, file.c_str());
	if (texture == nullptr){
		std::cout << "IMG_LoadTexture failed: " << file.c_str() << std::endl;
	}
	return texture;
}

/** 
-=-=-=-= renderTexture -=-=-=-=-
*/
void renderTexture( SDL_Texture *tex, SDL_Renderer *ren, int x, int y, int w, int h ){
	SDL_Rect dst;
	dst.w = w; dst.h = h; dst.x = x; dst.y = y;
	SDL_RenderCopy( ren, tex, NULL, &dst );
}

void renderTexture( SDL_Texture *tex, SDL_Renderer *ren, int x, int y ){
	int w, h;
	SDL_QueryTexture( tex, NULL, NULL, &w, &h );
	renderTexture( tex, ren, x, y, w, h );	
}

/**
-=-=-=-=-=- MAIN -=-=-=-=-=-=-=-
**/
int main(int argc, char** argv) {
	SDL_Event e;
	bool quit = false;
	
	int blitX=15, blitY= 15, xdir=1, ydir=1;
	
	
	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        logError( std::cout, "SDL_Init failed." );
        return 1;
    } logVerbose( std::cout, "SDL initialized..." );
    
    // initialize SDL image helper library
    if( (IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG) != IMG_INIT_PNG ){
    	logError( std::cout, "IMG_Init failed.");
		SDL_Quit();
		return 1;	
    } logVerbose( std::cout, "SDL_image initialized..." );
    
    // create a window
    SDL_Window *win = SDL_CreateWindow( WINDOW_TITLE, WINDOW_LEFT, WINDOW_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	if (win == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	} logVerbose( std::cout, "Window created." );
	
	// create a renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // second param sets preferred device, here we use -1 to let SDL choose the best one for us
	if (ren == nullptr){
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	} logVerbose( std::cout, "SDL renderer created." );
	
	// load a bitmap file
//	std::string imagePath = SDL_GetBasePath() + (std::string)"hello.bmp";
//	SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
//	if (bmp == nullptr){
//		SDL_DestroyRenderer(ren);
//		SDL_DestroyWindow(win);
//		std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
//		SDL_Quit();
//		return 1;
//	}
	
	// put bitmap into texture
//	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
//	SDL_FreeSurface(bmp);
//	if (tex == nullptr){
//		SDL_DestroyRenderer(ren);
//		SDL_DestroyWindow(win);
//		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
//		SDL_Quit();
//		return 1;
//	}

	// Use SDL_Image to directly load an image into a texture
	std::string pngPath = SDL_GetBasePath() + (std::string)"hello.png";
	SDL_Texture *tex = loadTexture( pngPath, ren );
	
	
//	//A sleepy rendering loop, wait for 3 seconds and render and present the screen each time
//	for (int i = 0; i < 3; ++i){
//		//First clear the renderer
//		SDL_RenderClear(ren);
//		//Draw the texture
////		SDL_RenderCopy(ren, tex, NULL, NULL);
//		renderTexture( tex, ren, 100, 100 );
//		//Update the screen
//		SDL_RenderPresent(ren);
//		//Take a quick break after all that hard work
//		SDL_Delay(1000);
//	}

	// render/event loop
	logVerbose( std::cout, "Entering main loop..." );
	while( !quit ){
		while( SDL_PollEvent( &e )){
			if( e.type == SDL_QUIT )			quit = true;
			if( e.type == SDL_KEYDOWN )			quit = true;
			if( e.type == SDL_MOUSEBUTTONDOWN )	quit = true;
		}
		
		// update bouncing image
		blitX += xdir;
		blitY += ydir;
		if( blitX < 0 || (blitX + 400 > SCREEN_WIDTH) ) xdir *= -1;
		if( blitY < 0 || (blitY + 400 > SCREEN_HEIGHT) ) ydir *= -1;
		
		// render
		SDL_RenderClear( ren );
		renderTexture( tex, ren, blitX, blitY );
		SDL_RenderPresent( ren );
	} logVerbose( std::cout, "Loop ended... beginning clean-up..." );
	
	// clean up and end	
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	IMG_Quit();
	SDL_Quit();

    return 0;
}
