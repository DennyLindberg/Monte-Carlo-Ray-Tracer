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

#include <thread>
struct ThreadInfo
{
	int threadId = 0;
	int xBegin = 0;
	int xEnd = 0;
	int yBegin = 0;
	int yEnd = 0;
};
/*
	const int numThreads = std::thread::hardware_concurrency();
	
	ApplicationClock performanceTimer;
	performanceTimer.Tick();

	std::vector<std::thread> t(numThreads);
	std::vector<double> maxValues(numThreads);
	for (int i = 0; i < numThreads; i++)
	{
		ThreadInfo info = { &buffer, &glImage, &maxValues[i], i, numThreads };
		t[i] = std::thread(DetermineMaxValue_threaded, info);
	}
	for (int i = 0; i < numThreads; i++)
	{
		t[i].join();
	}

	performanceTimer.Tick();
	std::cout << "Image conversion (" + (enableMultithreading ? (std::to_string(numThreads) + " threads") : "single thread") + "): " + std::to_string(performanceTimer.DeltaTime()) + " s\r\n";
*/

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 640;
static const unsigned int SCREEN_HEIGHT = 480;
static const unsigned int CHANNELS_PER_PIXEL = 4; // RGBA

static const bool RAY_TRACE_UNLIT = false;
static const unsigned int RAY_TRACE_DEPTH = 2;

static const float CAMERA_FOV = 90.0f;

static const float SCREEN_UPDATE_DELAY = 2.0f;
static const bool USE_MULTITHREADING = true;

int main()
{
	OpenGLWindow window("OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);
	window.Clear();

	GLFullscreenImage glImage(SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL);
	ColorDbl backgroundColor = ColorDbl( 0.0f, 0.0f, 0.0f, 1.0f );

	/*
		Initialize scene
	*/
	CornellBoxScene scene{2.0f, 2.0f, 2.0f};
	//HexagonScene scene;
	Camera camera = Camera{SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL, CAMERA_FOV};
	scene.MoveCameraToRecommendedPosition(camera);
	scene.AddExampleSpheres();
	scene.AddExampleLight({1.0f, 1.0f, 1.0f, 1.0f});
	scene.CacheLights();

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
			for (unsigned int y = 0; y < SCREEN_HEIGHT; ++y)
			{
				for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
				{
					cameraRay = camera.GetPixelRay(x + 0.5f, y + 0.5f);

					if constexpr(RAY_TRACE_UNLIT)
					{
						rayColor = scene.TraceUnlit(cameraRay);
					}
					else
					{
						rayColor = scene.TraceRay(cameraRay, RAY_TRACE_DEPTH);
					}

					if (rayColor.a != 0.0f) rayColor.a = 1.0f;
					else					rayColor = backgroundColor;

					camera.pixels.SetPixel(x, y, rayColor);
					glImage.buffer.SetPixel(x, y, rayColor.r, rayColor.g, rayColor.b, rayColor.a);
				}

				// Only upload every fifth row
				if (y % 5 == 0 || y == SCREEN_HEIGHT-1)
				{
					//window.Clear();
					glImage.Draw();
					window.SwapFramebuffer();
					lastScreenUpdate = clock.Time();
				}
			}
		}



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
