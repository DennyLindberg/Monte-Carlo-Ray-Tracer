/*
	Monte Carlo Ray Tracer implemented for the course TNCG15: Advanced Global Illumination and Rendering

	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "core.h"

static const int SCREEN_FULLSCREEN = 0;
static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;

int main()
{
	ApplicationClock clock;

	SDL_GLContext maincontext;
	SDL_Window* window = nullptr;
	init_main_window(window, maincontext, "OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN);

	Quad fullscreenQuad;
	ImageBuffer fullscreenImage(SCREEN_WIDTH, SCREEN_HEIGHT, 4);

	// Choose one of these two
	//fullscreenImage.FillDebug();
	fullscreenImage.LoadPNG("example_image.png");
	
	// Draw a blue square on the image
	fullscreenImage.FillSquare((unsigned int)(fullscreenImage.width()*0.2f), (unsigned int)(fullscreenImage.height()*0.2f),
							   (unsigned int)(fullscreenImage.width()*0.4f), (unsigned int)(fullscreenImage.height()*0.8f),
							   0, 0, 255, 255);

	SDL_Event event;
	bool quit = false;
	while (!quit) 
	{
		clock.Tick();
		
		std::string windowTitle = "FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time());
		SDL_SetWindowTitle(window, windowTitle.c_str());

		// Animate background
		float bgValue = abs(sin(clock.Time()));
		fillWindow(bgValue, bgValue, bgValue, 1.0f);

		fullscreenImage.Update();
		fullscreenImage.UseForDrawing();
		fullscreenQuad.Draw();

		SDL_GL_SwapWindow(window);
		while (SDL_PollEvent(&event)) 
		{
			if (event.type == SDL_QUIT) 
			{
				quit = true;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_s:
					TakeScreenshot("screenshot.png", SCREEN_WIDTH, SCREEN_HEIGHT);
					break;
				}
			}
		}
	}

	return 0;
}
