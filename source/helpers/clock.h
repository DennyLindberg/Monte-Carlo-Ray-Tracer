/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once

#include "SDL2/SDL.h"
#undef main

class ApplicationClock
{
protected:
	Uint64 currentTimeMS = 0;
	Uint64 previousTimeMS = 0;
	double _deltaTime = 0.0f;
	float _time = 0.0f;

public:
	ApplicationClock()
	{
		Tick();
	}

	~ApplicationClock() = default;

	void Tick()
	{
		previousTimeMS = currentTimeMS;
		currentTimeMS = SDL_GetPerformanceCounter();
		_deltaTime = (double)((currentTimeMS - previousTimeMS) / (double)SDL_GetPerformanceFrequency());
		_time = SDL_GetTicks() / 1000.0f;
	}

	float Time() { return _time; }
	double DeltaTime() { return _deltaTime; }
};
