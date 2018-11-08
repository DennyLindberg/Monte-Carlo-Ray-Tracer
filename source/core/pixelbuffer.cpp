/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "pixelbuffer.h"

PixelBuffer::PixelBuffer(unsigned int width, unsigned int height)
	: imageWidth{ width }, imageHeight{ height }
{
	dataSize = imageWidth * imageHeight * 3;
	data.resize(dataSize);
	rayCount.resize(dataSize);

	for (unsigned int i = 0; i < dataSize; ++i)
	{
		data[i] = 0.0;
		rayCount[i] = 0;
	}

	dx = 2.0 / (double)imageWidth;
	dy = 2.0 / (double)imageHeight;
	aspect = imageWidth / (double)imageHeight;
}

void PixelBuffer::SetPixel(unsigned int pixelIndex, double r, double g, double b)
{
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
}

void PixelBuffer::SetPixel(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] = color.r;
	data[pixelIndex + 1] = color.g;
	data[pixelIndex + 2] = color.b;
}

void PixelBuffer::Accumulate(unsigned int pixelIndex, ColorDbl color)
{
	data[pixelIndex] += color.r;
	data[pixelIndex + 1] += color.g;
	data[pixelIndex + 2] += color.b;

	rayCount[pixelIndex]++;
}

uint64_t PixelBuffer::GetRayCount(unsigned int pixelIndex)
{
	return rayCount[pixelIndex];
}

unsigned int PixelBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * 3 + x * 3;
}

ColorDbl PixelBuffer::GetPixelColor(unsigned int x, unsigned int y)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	return ColorDbl(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2]);
}
