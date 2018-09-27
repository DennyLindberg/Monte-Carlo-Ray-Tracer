#pragma once

#include <thread>
#include <atomic>
#include "core.h"

struct ThreadInfo
{
	PixelBuffer* buffer = nullptr;
	GLFullscreenImage* glImage = nullptr;
	std::atomic<double>* maxValue = nullptr;
	int threadId = 0;
	int numThreads = 0;
};

void DetermineMaxValue_threaded(ThreadInfo info)
{
	PixelBuffer& buffer = *info.buffer;
	GLImageBuffer& imageBuffer = info.glImage->buffer;
	int threadId = info.threadId;
	int numThreads = info.numThreads;
	std::atomic<double>& maxValue = *info.maxValue;

	// Determine percentage of range
	float threadBegin = (float)threadId / (float)numThreads;
	float threadEnd = (float)(threadId + 1) / (float)numThreads;

	int lastThread = numThreads - 1;
	int rangeStart = (threadId == 0) ? 0 : (int)floor(threadBegin * imageBuffer.size());
	int rangeEnd = (threadId == lastThread) ? imageBuffer.size() : (int)floor(threadEnd * imageBuffer.size());

	// Get max value of any channel for normalization during transfer
	for (int i = rangeStart; i < rangeEnd; ++i)
	{
		if (buffer[i] > maxValue)
		{
			maxValue = buffer[i];
		}
	}
}

void ConvertPixelBufferToGLImage_threaded(ThreadInfo info)
{
	PixelBuffer& buffer = *info.buffer;
	GLImageBuffer& imageBuffer = info.glImage->buffer;
	int threadId = info.threadId;
	int numThreads = info.numThreads;
	std::atomic<double>& maxValue = *info.maxValue;

	// Determine percentage of range
	float threadBegin = (float)threadId / (float)numThreads;
	float threadEnd = (float)(threadId + 1) / (float)numThreads;

	int lastThread = numThreads - 1;
	int rangeStart = (threadId == 0) ? 0 : (int)floor(threadBegin * imageBuffer.size());
	int rangeEnd = (threadId == lastThread) ? imageBuffer.size() : (int)floor(threadEnd * imageBuffer.size());

	// Convert buffer
	if (maxValue == 0.0)
	{
		// All black
		for (int i = rangeStart; i < rangeEnd; ++i)
		{
			imageBuffer[i] = 0;
		}
	}
	else
	{
		for (int i = rangeStart; i < rangeEnd; ++i)
		{
			// 64 bit [0.0, 1.0] -> 8 bit [0, 255]
			imageBuffer[i] = (GLubyte)round(buffer[i] / maxValue * 255);
		}
	}
}

void ConvertPixelBufferToGLImage(PixelBuffer& buffer, GLFullscreenImage& glImage)
{
	GLImageBuffer& imageBuffer = glImage.buffer;

	// Get max value of any channel for normalization during transfer
	double maxValue = 0.0;
	for (int i = 0; i < buffer.size(); ++i)
	{
		if (buffer[i] > maxValue)
		{
			maxValue = buffer[i];
		}
	}

	// Convert buffer
	if (maxValue == 0.0)
	{
		// All black
		for (int i = 0; i < buffer.size(); ++i)
		{
			imageBuffer[i] = 0;
		}
	}
	else
	{
		for (int i = 0; i < buffer.size(); ++i)
		{
			// 64 bit [0.0, 1.0] -> 8 bit [0, 255]
			imageBuffer[i] = (GLubyte)round(buffer[i] / maxValue * 255);
		}
	}
}

void UpdateUsingThreads(PixelBuffer& buffer, GLFullscreenImage& glImage, int numThreads)
{
	std::vector<std::thread> t(numThreads);
	std::atomic<double> maxValue = 0.0;
	for (int i = 0; i < numThreads; i++)
	{
		ThreadInfo info = { &buffer, &glImage, &maxValue, i, numThreads };
		t[i] = std::thread(DetermineMaxValue_threaded, info);
	}
	for (int i = 0; i < numThreads; i++)
	{
		t[i].join();
	}

	for (int i = 0; i < numThreads; i++)
	{
		ThreadInfo info = { &buffer, &glImage, &maxValue, i, numThreads };
		t[i] = std::thread(ConvertPixelBufferToGLImage_threaded, info);
	}
	for (int i = 0; i < numThreads; i++)
	{
		t[i].join();
	}
}

void CopyPixelsToImage(PixelBuffer& buffer, GLFullscreenImage& glImage, bool enableMultithreading)
{
	const int numThreads = std::thread::hardware_concurrency();

	ApplicationClock performanceTimer;
	performanceTimer.Tick();

	if (enableMultithreading && numThreads > 0)
	{
		UpdateUsingThreads(buffer, glImage, numThreads);
	}
	else
	{
		ConvertPixelBufferToGLImage(buffer, glImage);
	}

	performanceTimer.Tick();
	std::cout << "Image conversion (" + (enableMultithreading ? (std::to_string(numThreads) + " threads") : "single thread") + "): " + std::to_string(performanceTimer.DeltaTime()) + " s\r\n";
}