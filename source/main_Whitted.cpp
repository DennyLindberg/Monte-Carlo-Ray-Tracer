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
#include "imageconversion.h"

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 640;
static const unsigned int SCREEN_HEIGHT = 480;
static const unsigned int CHANNELS_PER_PIXEL = 4; // RGBA

static const unsigned int RAY_TRACE_DEPTH = 1;

static const float CAMERA_FOV = 90.0f;

static const float SCREEN_UPDATE_DELAY = 2.0f;
static const bool USE_MULTITHREADING = true;

int main()
{
	OpenGLWindow window("OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);
	window.Clear();

	GLFullscreenImage glImage(SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL);
	PixelBuffer pixels(SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL);

	ColorDbl backgroundColor = ColorDbl{ 0.0f, 0.0f, 0.0f, 1.0f };

	/*
		Initialize scene
	*/
	//CornellBoxScene scene{2.0f, 2.0f, 2.0f};
	HexagonScene scene;
	Camera camera = Camera{SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL, CAMERA_FOV};
	scene.MoveCameraToRecommendedPosition(camera);
	scene.AddExampleSpheres();

	/*
		Application loop
	*/
	bool alreadyRendered = false;

	ApplicationClock clock;
	float lastScreenUpdate = clock.Time();
	bool quit = false;
	while (!quit)
	{
		clock.Tick();
		window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));

		/*
			Ray tracing loop

			Exit loop once we have reached SCREEN_UPDATE_DELAY.
		*/
		//while ((clock.Time() - lastScreenUpdate) >= SCREEN_UPDATE_DELAY) 
		//{}

		if (!alreadyRendered)
		{
			alreadyRendered = true;

			/*
				Let's run path tracing once per pixel as a test
			*/
			Ray cameraRay;
			ColorDbl rayColor;
			for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
			{
				for (unsigned int y = 0; y < SCREEN_HEIGHT; ++y)
				{
					cameraRay = camera.GetPixelRay(x + 0.5f, y + 0.5f);
					rayColor = scene.TraceRay(cameraRay, RAY_TRACE_DEPTH);

					if (rayColor.a != 0.0f)
					{
						rayColor.a = 1.0f;
						camera.pixels.SetPixel(x, y, rayColor);
					}
					else
					{
						camera.pixels.SetPixel(x, y, backgroundColor);
					}
				}
			}

			/*
				Redraw screen with current ray tracing result
			*/
			lastScreenUpdate = clock.Time();
			CopyPixelsToImage(camera.pixels, glImage, USE_MULTITHREADING);
		}

		window.Clear();
		glImage.Draw();
		window.SwapFramebuffer();

		/*
			Handle input events
		*/
		SDL_Event event;
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
