#pragma once
#include "../core/math.h"
#include <vector>

class PixelBuffer
{
protected:
	std::vector<double> data;
	std::vector<uint64_t> rayCount;

	unsigned int dataSize = 0;
	unsigned int imageWidth = 0;
	unsigned int imageHeight = 0;

	double dx = 0.0;
	double dy = 0.0;
	double aspect = 1.0;

public:
	PixelBuffer(unsigned int width, unsigned int height);
	~PixelBuffer() = default;

	int size()		const { return dataSize; }
	int numPixels() const { return imageWidth * imageHeight; }
	int width()		const { return imageWidth; }
	int height()	const { return imageHeight; }

	// Returns the dimensions of each pixel in screen space coordinates
	double deltaX() const { return dx; }
	double deltaY() const { return dy; }
	double aspectRatio() const { return aspect; }

	inline double& operator[] (unsigned int i) { return data[i]; }
	void SetPixel(unsigned int pixelIndex, double r, double g, double b);
	void SetPixel(unsigned int pixelIndex, ColorDbl color);
	void Accumulate(unsigned int pixelIndex, ColorDbl color);
	uint64_t GetRayCount(unsigned int pixelIndex);
	unsigned int PixelArrayIndex(unsigned int x, unsigned int y);

	ColorDbl GetPixelColor(unsigned int x, unsigned int y);
};