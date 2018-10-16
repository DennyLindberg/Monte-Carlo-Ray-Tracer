/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "datatypes.h"

#include <iostream>
#include <memory>

void PixelBuffer::SetPixel(unsigned int x, unsigned int y, double r, double g, double b, double a)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
	data[pixelIndex + 3] = a;
}

void PixelBuffer::SetPixel(unsigned int x, unsigned int y, ColorDbl color)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	data[pixelIndex] = color.r;
	data[pixelIndex + 1] = color.g;
	data[pixelIndex + 2] = color.b;
	data[pixelIndex + 3] = color.a;
}

unsigned int PixelBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * imageChannelCount + x * imageChannelCount;
}