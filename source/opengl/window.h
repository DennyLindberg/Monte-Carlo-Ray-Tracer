/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once

#include "SDL2/SDL.h"
#undef main

#include "glad/glad.h"


void init_main_window(SDL_Window*& window, SDL_GLContext& maincontext, const char* caption, int screenWidth, int screenHeight, bool fullscreen)
{
	auto sdl_die = [](const char* message) {
		fprintf(stderr, "%s: %s\n", message, SDL_GetError());
		exit(2);
	};

	// Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		sdl_die("Couldn't initialize SDL");
	}

	atexit(SDL_Quit);
	SDL_GL_LoadLibrary(NULL); // Default OpenGL is fine.

	// Request an OpenGL 4.5 context (should be core)
	//SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create the window
	if (fullscreen)
	{
		window = SDL_CreateWindow(
			caption,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL
		);
	}
	else
	{
		window = SDL_CreateWindow(
			caption,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			screenWidth, screenHeight, SDL_WINDOW_OPENGL
		);
	}
	if (window == NULL) sdl_die("Couldn't set video mode");

	maincontext = SDL_GL_CreateContext(window);
	if (maincontext == NULL)
	{
		sdl_die("Failed to create OpenGL context");
	}

	// Check OpenGL properties
	printf("OpenGL loaded\n");
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	// Use v-sync
	SDL_GL_SetSwapInterval(1);

	// Disable depth test and face culling.
	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
}

void fillWindow(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}