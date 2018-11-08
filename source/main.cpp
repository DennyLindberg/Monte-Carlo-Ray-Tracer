/*
	Monte Carlo Ray Tracer implemented for the course TNCG15: Advanced Global Illumination and Rendering

	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

// STL includes
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <algorithm>
#include <thread>
#include <atomic>

// Application includes
#include "opengl/window.h"
#include "opengl/data.h"
#include "opengl/screenshot.h"
#include "helpers/clock.h"
#include "scene.h"
#include "helpers/OBJ_Loader.h"
#include "core/randomization.h"

UniformRandomGenerator uniformGenerator;

bool quit = false;

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 320;
static const unsigned int SCREEN_HEIGHT = 240;
static const float SCREEN_UPDATE_DELAY = 0.1f;

static const float CAMERA_FOV = 90.0f;

static const bool RAY_TRACE_UNLIT = false;
static const bool RAY_TRACE_RANDOM = true;
static const unsigned int RAY_TRACE_DEPTH = 20;
static const unsigned int RAY_COUNT_PER_PIXEL = RAY_TRACE_UNLIT ? 1 : 1;
static const unsigned int RAY_TRACE_LIGHT_SAMPLE_COUNT = 1;

static const bool APPLY_TONE_MAPPING = true;
static const bool USE_SIMPLE_TONE_MAPPER = true;
static const double TONE_MAP_GAMMA = 2.2;
static const double TONE_MAP_EXPOSURE = 1.0;


static const bool USE_MULTITHREADING = true;
struct ThreadInfo
{
	unsigned int id = 0;
	Camera* camera = nullptr;
	Scene* scene = nullptr;
	GLFullscreenImage* glImage = nullptr;
	bool isDone = false;
};
const unsigned int NUM_SUPPORTED_THREADS = std::thread::hardware_concurrency();
std::vector<std::thread> threads(NUM_SUPPORTED_THREADS);
std::vector<ThreadInfo> threadInfos(NUM_SUPPORTED_THREADS);
std::vector<UniformRandomGenerator> uniformGenerators(NUM_SUPPORTED_THREADS);

/*
	Main ray tracing function
*/
std::atomic_uint threaded_currentPixelIndex = 0;
inline bool GetNextPixelToRender(unsigned int& nextIndex, unsigned int& x, unsigned int& y, unsigned int threadId, PixelBuffer& buffer)
{
	if constexpr (RAY_TRACE_RANDOM)
	{
		// Allow threads to work on the whole image concurrently.
		// The likelyhood that two threads will write to the same pixel output is miniscule.
		// -> Two threads get the same index at different times during computation
		// -> Threads work with different rays and terminate randomly with Russian roulette
		// -> Threads would then have to finish at the same time and access/write the same memory
		// -> How miniscule is the chance for this to happen? Hit by lighting a thousand times the same day?
		x = (unsigned int)(uniformGenerator.RandomFloat(0.0f, float(SCREEN_WIDTH)));
		y = (unsigned int)(uniformGenerator.RandomFloat(0.0f, float(SCREEN_HEIGHT)));

		x = std::min(x, SCREEN_WIDTH - 1);
		y = std::min(y, SCREEN_HEIGHT - 1);

		nextIndex = buffer.PixelArrayIndex(x, y);
		return true;
	}
	else
	{
		// Threads will chase the next free pixel available.
		nextIndex = threaded_currentPixelIndex++;
		x = nextIndex % SCREEN_WIDTH;
		y = nextIndex / SCREEN_WIDTH;

		nextIndex *= 3;
		return (int(nextIndex) < buffer.size());
	}

}

bool RayTraceNextPixel(unsigned int threadId)
{
	ThreadInfo& thread = threadInfos[threadId];
	unsigned int pixelIndex = 0;
	unsigned int x = 0;
	unsigned int y = 0;

	if (!GetNextPixelToRender(pixelIndex, x, y, thread.id, thread.camera->pixels))
	{
		return false;
	}

	Camera& camera = *thread.camera;
	Scene& scene = *thread.scene;
	GLFullscreenImage& glImage = *thread.glImage;

	// Run trace for all rays
	Ray cameraRay;
	ColorDbl rayColor;
	int rayCount = RAY_COUNT_PER_PIXEL;
	float sx = 0.0f;
	float sy = 0.0f;
	while (--rayCount >= 0)
	{
		if constexpr (RAY_TRACE_UNLIT)
		{
			cameraRay = camera.GetPixelRay(float(x) + 0.5f, float(y) + 0.5f);
		}
		else
		{
			sx = uniformGenerator.RandomFloat();
			sy = uniformGenerator.RandomFloat();
			cameraRay = camera.GetPixelRay(float(x) + sx, float(y) + sy);
		}

		if constexpr (RAY_TRACE_UNLIT) rayColor = scene.TraceUnlit(cameraRay);
		else						   rayColor = scene.TraceRay(cameraRay, uniformGenerators[thread.id], RAY_TRACE_DEPTH);

		camera.pixels.Accumulate(pixelIndex, rayColor);
	}

	// When done, normalize colors
	ColorDbl outputColor = camera.pixels.GetPixelColor(x, y) / double(camera.pixels.GetRayCount(pixelIndex));

	if constexpr (!APPLY_TONE_MAPPING)
	{
		glImage.buffer.SetPixel(x, y, outputColor.r, outputColor.g, outputColor.b, 1.0);
	}
	else
	{
		if constexpr (USE_SIMPLE_TONE_MAPPER)
		{
			// Reinhard Tone Mapping
			outputColor = outputColor / (outputColor + ColorDbl(1.0));
			outputColor = pow(outputColor, ColorDbl(1.0 / TONE_MAP_GAMMA));
		}
		else
		{
			// Exposure tone mapping
			outputColor = ColorDbl(1.0) - glm::exp(-outputColor * TONE_MAP_EXPOSURE);
			outputColor = pow(outputColor, ColorDbl(1.0 / TONE_MAP_GAMMA));
		}

		glImage.buffer.SetPixel(x, y, outputColor.r, outputColor.g, outputColor.b, 1.0);
	}

	return true;
}

bool TracePixels(unsigned int threadId = 0)
{
	if (threadId == 0)
	{
		// Don't loop the main thread
		return RayTraceNextPixel(threadId);
	}
	else
	{
		// Extra threads continue to run independently
		while (RayTraceNextPixel(threadId) && !quit) {}
		threadInfos[threadId].isDone = true;

		return true;
	}
}

bool ThreadsAreDone()
{
	if constexpr (RAY_TRACE_RANDOM)
	{
		return false;
	}
	else if constexpr (USE_MULTITHREADING)
	{
		for (unsigned int i = 0; i < threadInfos.size(); i++)
		{
			if (!threadInfos[i].isDone)
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		return threadInfos[0].isDone;
	}
}

std::string TimeString(float time)
{
	int seconds = int(time);
	int minutes = seconds / 60;
	int hours = minutes / 60;
	float decimals = time - float(seconds);

	return std::to_string(hours) + "h " + std::to_string(minutes % 60) + "m " + std::to_string(seconds % 60) + "." + std::to_string(int(decimals*10.0f)) + "s";
}
std::string FpsString(float deltaTime) { return std::to_string(int(round(1.0f / deltaTime))); }

int main()
{
	OpenGLWindow window("OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);
	window.Clear();
	window.SwapFramebuffer();
	window.Clear();
	window.SwapFramebuffer();

	GLFullscreenImage glImage(SCREEN_WIDTH, SCREEN_HEIGHT);

	/*
		Initialize scene
	*/
	Camera camera = Camera{ SCREEN_WIDTH, SCREEN_HEIGHT, CAMERA_FOV };

	//CornellBoxScene scene{ 10.0f, 10.0f, 10.0f };
	HexagonScene scene;
	scene.backgroundColor = { 0.0f, 0.0f, 0.0f };
	scene.LIGHT_SUBSAMPLE_COUNT = RAY_TRACE_LIGHT_SAMPLE_COUNT;
	scene.MoveCameraToRecommendedPosition(camera);
	scene.AddExampleSpheres();
	scene.AddExampleLight(ColorDbl{ 100.0f });
	scene.PrepareForRayTracing();
	//scene.octree.PrintDebug();


	/*
		Application loop
	*/
	for (unsigned int i = 0; i < NUM_SUPPORTED_THREADS; i++)
	{
		threadInfos[i] = { i, &camera, &scene, &glImage, false };
	}

	if (USE_MULTITHREADING)
	{
		// Start all threads except the main one
		for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
		{
			threads[i] = std::thread(TracePixels, i);
		}
	}

	ThreadInfo& mainThread = threadInfos[0];
	ApplicationClock clock;
	float lastScreenUpdate = clock.Time();
	float screenUpdateDelta = 0.0f;
	bool threadsAreDone = false;
	while (!quit)
	{
		if (!threadsAreDone)
		{
			if (!mainThread.isDone)
			{
				mainThread.isDone = !TracePixels();
			}

			clock.Tick();
			screenUpdateDelta = clock.Time() - lastScreenUpdate;

			threadsAreDone = ThreadsAreDone();
			if (threadsAreDone || (screenUpdateDelta >= SCREEN_UPDATE_DELAY))
			{
				window.SetTitle("Time: " + TimeString(clock.Time()) + ", FPS: " + FpsString(screenUpdateDelta));
				glImage.Draw();
				window.SwapFramebuffer();
				lastScreenUpdate = clock.Time();

				if (threadsAreDone)
				{
					std::cout << "\r\n\r\nRender finished at " + TimeString(clock.Time()) + "\r\n";
				}
			}
		}


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
		for (unsigned int i = 1; i < NUM_SUPPORTED_THREADS; i++)
		{
			threads[i].join();
		}
	}


	return 0;
}
