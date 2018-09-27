/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#pragma once
#include <vector>
#include "glad/glad.h"

class GLImageBuffer
{
protected:
	std::vector<GLubyte> data;
	GLuint textureId = 0;
	GLenum pixelFormat = GL_RGBA;

	int dataSize = 0;
	int imageWidth = 0;
	int imageHeight = 0;
	int imageChannelCount = 0;

public:
	GLImageBuffer(int width, int height, int channels, GLenum format = GL_RGBA)
		: imageWidth{ width }, imageHeight{ height }, imageChannelCount{ channels }, pixelFormat{ format }
	{
		dataSize = imageWidth * imageHeight * imageChannelCount;
		data.resize(dataSize);

		glGenTextures(1, &textureId);
		UpdateParameters();
	}

	~GLImageBuffer()
	{
		glDeleteTextures(1, &textureId);
	}

	void UpdateParameters();

	int size() { return dataSize; }
	int numPixels() { return imageWidth * imageHeight; }
	int width() { return imageWidth; }
	int height() { return imageHeight; }

	inline GLubyte& operator[] (unsigned int i) { return data[i]; }
	void SetPixel(unsigned int x, unsigned int y, GLubyte r, GLubyte g, GLubyte b, GLubyte a);
	unsigned int PixelArrayIndex(unsigned int x, unsigned int y);
	void UseForDrawing();
	void SendToGPU();
	void FillSquare(unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY,
		GLubyte r, GLubyte g, GLubyte b, GLubyte a);

	void FillDebug();
	void SaveAsPNG(std::string filename, bool incrementNewFile = false);
	void LoadPNG(std::string filename);
};

class GLQuad
{
protected:
	GLuint positionBuffer = 0;
	GLuint texCoordBuffer = 0;

	GLuint glProgram = 0;

public:
	GLQuad();

	~GLQuad();

	void Draw();

protected:
	void CreateMeshBuffer();

	template <class T>
	void BufferVector(GLenum glBufferType, const std::vector<T>& vector, GLenum usage)
	{
		if (vector.size() > 0)
		{
			glBufferData(glBufferType, vector.size() * sizeof(T), &vector.front(), usage);
		}
		else
		{
			glBufferData(glBufferType, 0, NULL, usage);
		}
	}

	void CreateShaders();
};

class GLFullscreenImage
{
protected:
	GLQuad mesh;

public:
	GLImageBuffer buffer;

	GLFullscreenImage(int width, int height, int channels, GLenum format = GL_RGBA)
		: buffer{ width, height, channels, format }
	{}

	void Draw()
	{
		buffer.UseForDrawing();
		buffer.SendToGPU();
		mesh.Draw();
	}
};