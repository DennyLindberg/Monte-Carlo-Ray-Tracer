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

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 640;
static const unsigned int SCREEN_HEIGHT = 480;
static const unsigned int CHANNELS_PER_PIXEL = 4; // RGBA

static const bool RAY_TRACE_UNLIT = false;
static const unsigned int RAY_TRACE_DEPTH = 2;

static const float CAMERA_FOV = 90.0f;

static const float SCREEN_UPDATE_DELAY = 0.0f;

static const bool USE_MULTITHREADING = true;
typedef std::vector<std::thread> ThreadVector;
const unsigned int NUM_SUPPORTED_THREADS = std::thread::hardware_concurrency();
ThreadVector threads(NUM_SUPPORTED_THREADS);

/*
	Multi-threading details
*/
struct ThreadInfo
{
	unsigned int id = 0;
	Camera* camera = nullptr;
	Scene* scene = nullptr;
	GLFullscreenImage* glImage = nullptr;
	bool loop = false;
};

void StopThreads(ThreadVector& threads)
{
	for (std::thread& thread : threads)
	{
		thread.~thread();
	}
}

/*
	Main ray tracing function
*/
void TraceRegion(unsigned int yBegin, unsigned int yEnd, Camera& camera, Scene& scene, GLFullscreenImage& glImage)
{
	Ray cameraRay;
	ColorDbl rayColor;

	for (unsigned int y = yBegin; y < yEnd; ++y)
	{
		for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
		{
			cameraRay = camera.GetPixelRay(x + 0.5f, y + 0.5f);

			if constexpr (RAY_TRACE_UNLIT)
			{
				rayColor = scene.TraceUnlit(cameraRay);
			}
			else
			{
				rayColor = scene.TraceRay(cameraRay, RAY_TRACE_DEPTH);
			}

			camera.pixels.SetPixel(x, y, rayColor);
			glImage.buffer.SetPixel(x, y, rayColor.r, rayColor.g, rayColor.b, 1.0);
		}
	}
}

void TraceRegionThreaded(ThreadInfo thread)
{
	unsigned int yStep = SCREEN_HEIGHT / NUM_SUPPORTED_THREADS;

	unsigned int yBegin = yStep* thread.id;
	unsigned int yEnd = yBegin + yStep;

	if (thread.id == NUM_SUPPORTED_THREADS - 1)
	{
		yEnd = SCREEN_HEIGHT;
	}

	do
	{
		TraceRegion(yBegin, yEnd, *thread.camera, *thread.scene, *thread.glImage);
	} while (thread.loop);
}

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
		if (!alreadyRendered)
		{
			alreadyRendered = true;
			lastScreenUpdate = clock.Time();

			if (!USE_MULTITHREADING)
			{
				TraceRegion(0, SCREEN_HEIGHT, camera, scene, glImage);
				clock.Tick();
			}
			else
			{
				// Additional threads are created from id=1 because id=0 is the main thread.
				for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
				{
					ThreadInfo info = { i, &camera, &scene, &glImage, false };
					threads[i] = std::thread(TraceRegionThreaded, info);
				}

				/*
					Main thread has two jobs:
						1. Render the first region
						2. Update the screen

					It will update the screen after each rendered row.
				*/
				unsigned int yStep = SCREEN_HEIGHT / NUM_SUPPORTED_THREADS;
				for (unsigned int y = 1; y < yStep; y++)
				{
					TraceRegion(y, y + 1, camera, scene, glImage);

					clock.Tick();
					if ((clock.Time() - lastScreenUpdate) >= SCREEN_UPDATE_DELAY)
					{
						window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
						glImage.Draw();
						window.SwapFramebuffer();
						lastScreenUpdate = clock.Time();
					}
				}
			
				// When main thread is done, wait for other threads to catch up
				for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
				{
					threads[i].join();
				}
			}

			window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
		}

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

	//StopThreads();
	return 0;
}
