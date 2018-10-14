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

static const float CAMERA_FOV = 45.0f;

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
	Camera camera = Camera{SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS_PER_PIXEL, CAMERA_FOV};
	camera.transform.position = vec4(0.0f, 0.0f, 5.0f, 1.0f);
	camera.LookAt(vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f));
	
	Scene scene;
	SphereObject* sphere1 = scene.CreateObject<SphereObject>();
	SphereObject* sphere2 = scene.CreateObject<SphereObject>();
	SphereObject* sphere3 = scene.CreateObject<SphereObject>();

	sphere1->transform.position = vec4(0.0, 0.0, 0.0, 1.0);
	sphere2->transform.position = vec4(-2.0, 0.0, -2.0, 1.0);
	sphere3->transform.position = vec4(2.0, 0.0, 2.0, 1.0);

	sphere1->radius = 0.5f;
	sphere2->radius = 0.5f;
	sphere3->radius = 0.5f;

	sphere1->color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	sphere2->color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	sphere3->color = vec4(0.0f, 0.0f, 1.0f, 1.0f);

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
			Ray initialRay;
			SceneObject* hitResult;
			float hitDistance;
			ColorDbl pixelColor;
			for (unsigned int x = 0; x < SCREEN_WIDTH; ++x)
			{
				for (unsigned int y = 0; y < SCREEN_HEIGHT; ++y)
				{
					initialRay = camera.EmitRayThroughPixelCenter(x, y);

					if (scene.IntersectRay(initialRay, hitResult, hitDistance))
					{
						float fakeDepth = abs((camera.transform.position.z + 1.0 - hitDistance) / camera.transform.position.z);

						pixelColor = hitResult->color;
						pixelColor *= fakeDepth;
						pixelColor.a = 1.0f;
						camera.pixels.SetPixel(x, y, pixelColor);
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

			window.Clear();
			CopyPixelsToImage(camera.pixels, glImage, USE_MULTITHREADING);
			glImage.Draw();
			window.SwapFramebuffer();
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
