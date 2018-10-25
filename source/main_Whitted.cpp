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

bool quit = false;

static const bool SCREEN_VSYNC = false;
static const unsigned int SCREEN_FULLSCREEN = 0;
static const unsigned int SCREEN_WIDTH = 640;
static const unsigned int SCREEN_HEIGHT = 480;
static const float SCREEN_UPDATE_DELAY = 0.1f;

static const float CAMERA_FOV = 90.0f;

static const bool RAY_TRACE_UNLIT = false;
static const bool RAY_TRACE_RANDOM = true;
static const unsigned int RAY_TRACE_DEPTH = 5;
static const unsigned int RAY_COUNT_PER_PIXEL = RAY_TRACE_UNLIT? 1 : 1;
static const unsigned int RAY_TRACE_LIGHT_SAMPLE_COUNT = 4;

static const bool APPLY_TONE_MAPPING = true;
static const bool USE_SIMPLE_TONE_MAPPER = true;
static const double TONE_MAP_GAMMA = 2.2;
static const double TONE_MAP_EXPOSURE = 1.0;

static const bool USE_MULTITHREADING = true;
typedef std::vector<std::thread> ThreadVector;
const unsigned int NUM_SUPPORTED_THREADS = std::thread::hardware_concurrency();
ThreadVector threads(NUM_SUPPORTED_THREADS);

struct ThreadInfo
{
	unsigned int id = 0;
	Camera* camera = nullptr;
	Scene* scene = nullptr;
	GLFullscreenImage* glImage = nullptr;
	bool loop = false;
	bool isDone = false;
};
std::vector<ThreadInfo> threadInfos(NUM_SUPPORTED_THREADS);

/*
	Main ray tracing function
*/
inline void TraceSubPixels(unsigned int x, unsigned int y, Camera& camera, Scene& scene, GLFullscreenImage& glImage)
{
	// Each pixel will trace a certain number of random rays in random directions (subpixels)
	unsigned int pixelIndex = camera.pixels.PixelArrayIndex(x, y);
	int rayCount = RAY_COUNT_PER_PIXEL;
	float sx = 0.0f;
	float sy = 0.0f;

	// Run trace for all rays
	Ray cameraRay;
	ColorDbl rayColor;
	while (--rayCount >= 0)
	{
		if constexpr(RAY_TRACE_UNLIT)
		{
			cameraRay = camera.GetPixelRay(x + 0.5f, y + 0.5f);
		}
		else
		{
			sx = uniformGenerator.RandomFloat();
			sy = uniformGenerator.RandomFloat();
			cameraRay = camera.GetPixelRay(x + sx, y + sy);
		}


		if constexpr (RAY_TRACE_UNLIT) rayColor = scene.TraceUnlit(cameraRay);
		else						   rayColor = scene.TraceRay(cameraRay, RAY_TRACE_DEPTH);
		
		camera.pixels.AddRayColor(pixelIndex, rayColor);
	}

	// When done, normalize colors
	ColorDbl outputColor = camera.pixels.GetPixelColor(x, y) / double(camera.pixels.GetRayCount(pixelIndex));

	if constexpr(!APPLY_TONE_MAPPING)
	{
		glImage.buffer.SetPixel(x, y, outputColor.r, outputColor.g, outputColor.b, 1.0);
	}
	else
	{
		if constexpr(USE_SIMPLE_TONE_MAPPER)
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

}

inline void Trace(unsigned int yBegin, unsigned int yEnd, Camera& camera, Scene& scene, GLFullscreenImage& glImage)
{
	if constexpr (RAY_TRACE_RANDOM)
	{

		unsigned int x = (unsigned int)(uniformGenerator.RandomFloat(0.0f, float(SCREEN_WIDTH)));
		unsigned int y = (unsigned int)(uniformGenerator.RandomFloat(float(yBegin), float(yEnd)));
		TraceSubPixels(x, y, camera, scene, glImage);
	}
	else
	{
		for (unsigned int y = yBegin; y < yEnd; ++y)
		{
			for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
			{
				TraceSubPixels(x, y, camera, scene, glImage);
			}
		}
	}
}

void TraceThreaded(ThreadInfo& thread)
{
	unsigned int yStep = SCREEN_HEIGHT / NUM_SUPPORTED_THREADS;

	unsigned int yBegin = yStep* thread.id;
	unsigned int yEnd = yBegin + yStep;

	if (thread.id == NUM_SUPPORTED_THREADS - 1)
	{
		yEnd = SCREEN_HEIGHT-1;
	}

	do
	{
		Trace(yBegin, yEnd, *thread.camera, *thread.scene, *thread.glImage);
	} while (thread.loop && !quit);

	thread.isDone = true;
}

bool ThreadsAreDone(std::vector<ThreadInfo>& threads)
{
	// Don't include the main thread
	for (unsigned int i=1; i<threads.size(); i++)
	{
		if (!threads[i].isDone)
		{
			return false;
		}
	}
	return true;
}

int main()
{
	OpenGLWindow window("OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);
	window.Clear();
    window.SwapFramebuffer();
    window.Clear();

	GLFullscreenImage glImage(SCREEN_WIDTH, SCREEN_HEIGHT);

	/*
		Initialize scene
	*/
	//CornellBoxScene scene{2.0f, 2.0f, 2.0f};
	HexagonScene scene;
	Camera camera = Camera{SCREEN_WIDTH, SCREEN_HEIGHT, CAMERA_FOV};
	scene.MoveCameraToRecommendedPosition(camera);
	scene.AddExampleSpheres();
	scene.AddExampleLight(ColorDbl{1.0f});
	scene.CacheLights();
	scene.backgroundColor = {0.0f, 0.0f, 0.0f};
	scene.LIGHT_SAMPLE_COUNT = RAY_TRACE_LIGHT_SAMPLE_COUNT;

	/*
		Application loop
	*/
	ApplicationClock clock;
	float lastScreenUpdate = clock.Time();
	if (USE_MULTITHREADING)
	{
		for (unsigned int i = 0; i < NUM_SUPPORTED_THREADS; i++)
		{
			threadInfos[i] = { i, &camera, &scene, &glImage, RAY_TRACE_RANDOM, false };

			// Additional threads are created from id=1 because id=0 is the main thread.
			if (i > 0)
			{
				threads[i] = std::thread(TraceThreaded, threadInfos[i]);
			}
		}
	}

	unsigned int y = 0;
	unsigned int mainThreadYMax = USE_MULTITHREADING ? SCREEN_HEIGHT / NUM_SUPPORTED_THREADS : SCREEN_HEIGHT;
	bool mainThreadTracing = false;
	while (!quit)
	{
		if constexpr (RAY_TRACE_RANDOM)
		{
			Trace(0, mainThreadYMax, camera, scene, glImage);
		}
		else 
		{
			mainThreadTracing = y < mainThreadYMax;
			if (mainThreadTracing)
			{
				Trace(y, y + 1, camera, scene, glImage);
				y++;
			}
		}

		clock.Tick();
		if ((clock.Time() - lastScreenUpdate) >= SCREEN_UPDATE_DELAY)
		{
			if constexpr (!RAY_TRACE_RANDOM)
			{
				if (mainThreadTracing && !ThreadsAreDone(threadInfos))
				{
					window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
				}
			}
			else
			{
				window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
			}

			glImage.Draw();
			window.SwapFramebuffer();
			lastScreenUpdate = clock.Time();
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
