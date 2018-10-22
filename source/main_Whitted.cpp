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

UniformRandomGenerator uniformGenerator;

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 640;
static const unsigned int SCREEN_HEIGHT = 480;
static const unsigned int CHANNELS_PER_PIXEL = 4; // RGBA

static const bool RAY_TRACE_UNLIT = false;
static const unsigned int RAY_TRACE_DEPTH = 3;
static const unsigned int RAY_COUNT_PER_STEP = 1;

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
void TraceRegionUnlit(unsigned int yBegin, unsigned int yEnd, Camera& camera, Scene& scene, GLFullscreenImage& glImage)
{
	Ray cameraRay;
	ColorDbl rayColor;
	unsigned int pixelIndex = 0;
	for (unsigned int y = yBegin; y < yEnd; ++y)
	{
		for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
		{
			pixelIndex = camera.pixels.PixelArrayIndex(x, y);
			cameraRay = camera.GetPixelRay(x + 0.5f, y + 0.5f);

			rayColor = scene.TraceUnlit(cameraRay);

			camera.pixels.SetPixel(pixelIndex, rayColor);
			glImage.buffer.SetPixel(pixelIndex, rayColor.r, rayColor.g, rayColor.b, 1.0);
		}
	}
}

void TraceRegion(unsigned int yBegin, unsigned int yEnd, Camera& camera, Scene& scene, GLFullscreenImage& glImage)
{
	// Pick a random pixel to trace
	unsigned int x = unsigned int(uniformGenerator.RandomFloat(0.0f, float(SCREEN_WIDTH)));
	unsigned int y = unsigned int(uniformGenerator.RandomFloat(yBegin, yEnd));
	unsigned int pixelIndex = camera.pixels.PixelArrayIndex(x, y);

	// Each pixel will trace a certain number of random rays in random directions (subpixels)
	int rayCount = RAY_COUNT_PER_STEP;
	float sx = 0.0f;
	float sy = 0.0f;

	// Run trace for all rays
	Ray cameraRay;
	ColorDbl rayColor;
	do
	{
		sx = uniformGenerator.RandomFloat();
		sy = uniformGenerator.RandomFloat();

		cameraRay = camera.GetPixelRay(x + sx, y + sy);
		rayColor = scene.TraceRay(cameraRay, RAY_TRACE_DEPTH);

		camera.pixels.AddRayColor(pixelIndex, rayColor);
	} while (--rayCount >= 0);

	// When done, normalize colors
	ColorDbl outputColor = camera.pixels.GetPixelColor(x, y) / double(camera.pixels.GetRayCount(pixelIndex));
	glImage.buffer.SetPixel(pixelIndex, outputColor.r, outputColor.g, outputColor.b, 1.0);
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
		if constexpr (RAY_TRACE_UNLIT)
		{
			TraceRegionUnlit(yBegin, yEnd, *thread.camera, *thread.scene, *thread.glImage);
		}
		else
		{
			TraceRegion(yBegin, yEnd, *thread.camera, *thread.scene, *thread.glImage);
		}
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

	if (RAY_TRACE_UNLIT)
	{
		while (!quit)
		{
			if (!alreadyRendered)
			{
				if (!USE_MULTITHREADING)
				{
					TraceRegion(0, SCREEN_HEIGHT, camera, scene, glImage);
					clock.Tick();
				}
				else
				{
					alreadyRendered = true;
					lastScreenUpdate = clock.Time();
					/*
					Main thread has two jobs:
					1. Render the first region
					2. Update the screen

					It will update the screen after each rendered row.
					*/

					// Additional threads are created from id=1 because id=0 is the main thread.
					for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
					{
						ThreadInfo info = { i, &camera, &scene, &glImage, false };
						threads[i] = std::thread(TraceRegionThreaded, info);
					}

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
			}

			glImage.Draw();
			window.SwapFramebuffer();
			lastScreenUpdate = clock.Time();

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
	}
	else
	{
		if (USE_MULTITHREADING)
		{
			// Additional threads are created from id=1 because id=0 is the main thread.
			for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
			{
				ThreadInfo info = { i, &camera, &scene, &glImage, true };
				threads[i] = std::thread(TraceRegionThreaded, info);
			}
		}

		unsigned int mainThreadYMax = USE_MULTITHREADING ? SCREEN_HEIGHT / NUM_SUPPORTED_THREADS : SCREEN_HEIGHT;
		while (!quit)
		{
			TraceRegion(0, mainThreadYMax, camera, scene, glImage);
			clock.Tick();
			window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
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

		if (USE_MULTITHREADING)
		{
			StopThreads(threads);
		}
	}

	return 0;
}
