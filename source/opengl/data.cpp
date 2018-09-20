/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "data.h"

#include <iostream>
#include <memory>
#include "../thirdparty/lodepng.h"

void ImageBuffer::UpdateParameters()
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0, pixelFormat, GL_UNSIGNED_BYTE, (GLvoid*)data.data());
}

void ImageBuffer::SetPixel(unsigned int x, unsigned int y, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	unsigned int pixelIndex = PixelArrayIndex(x, y);
	data[pixelIndex] = r;
	data[pixelIndex + 1] = g;
	data[pixelIndex + 2] = b;
	data[pixelIndex + 3] = a;
}

unsigned int ImageBuffer::PixelArrayIndex(unsigned int x, unsigned int y)
{
	return y * imageWidth * imageChannelCount + x * imageChannelCount;
}

void ImageBuffer::UseForDrawing()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);
}

void ImageBuffer::Update()
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, pixelFormat, GL_UNSIGNED_BYTE, (GLvoid*)data.data());
}

void ImageBuffer::FillSquare(unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY,
	GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	for (unsigned int x = startX; x <= endX; ++x)
	{
		for (unsigned int y = startY; y <= endY; ++y)
		{
			SetPixel(x, y, r, g, b, a);
		}
	}
}

void ImageBuffer::FillDebug()
{
	for (int x = 0; x < imageWidth; ++x)
	{
		for (int y = 0; y < imageHeight; ++y)
		{
			GLubyte r = (GLubyte)(x / (float)imageWidth * 255);
			GLubyte g = (GLubyte)(y / (float)imageHeight * 255);
			GLubyte b = 0;
			GLubyte a = 255;
			SetPixel(x, y, r, g, b, a);
		}
	}
}

void ImageBuffer::SaveAsPNG(std::string filename, bool incrementNewFile)
{
	unsigned error = lodepng::encode(filename, data, (unsigned int)imageWidth, (unsigned int)imageHeight);
	if (error)
	{
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
}

void ImageBuffer::LoadPNG(std::string filename)
{
	unsigned sourceWidth, sourceHeight;

	data.clear();
	data.shrink_to_fit();

	std::vector<unsigned char> png;
	lodepng::State state;
	unsigned error = lodepng::load_file(png, filename);
	if (!error)
	{
		error = lodepng::decode(data, sourceWidth, sourceHeight, state, png);
	}

	if (error)
	{
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
	else
	{
		const LodePNGColorMode& color = state.info_png.color;
		switch (color.colortype)
		{
		case LCT_RGB:
			pixelFormat = GL_RGB;
			break;
		case LCT_RGBA:
		default:
			pixelFormat = GL_RGBA;
			break;
		}

		imageWidth = sourceWidth;
		imageHeight = sourceHeight;
		imageChannelCount = lodepng_get_channels(&color);

		dataSize = imageWidth * imageHeight * imageChannelCount;

		UpdateParameters();
	}
}

Quad::Quad()
{
	CreateMeshBuffer();
	CreateShaders();
}

Quad::~Quad()
{
	glDeleteProgram(glProgram);
	glDeleteBuffers(1, &positionBuffer);
	glDeleteBuffers(1, &texCoordBuffer);
}

void Quad::Draw()
{
	const GLuint QUAD_NUM_VERTICES = 6; // two triangles
	glUseProgram(glProgram);
	glDrawArrays(GL_TRIANGLES, 0, QUAD_NUM_VERTICES);
}

void Quad::CreateMeshBuffer()
{
	// GL coordinate system is not like most UIs where the origin is upper left corner.
	// Y is up, X is right 
	float scale = 1.0f;
	float right = 1.0f * scale;
	float left = -1.0f * scale;
	float top = 1.0f * scale;
	float bottom = -1.0f * scale;

	const GLuint valuesPerPosition = 3;
	std::vector<float> positions = {
		// Triangle 1
		left, top, 0.0f,
		left, bottom, 0.0f,
		right, bottom, 0.0f,

		// Triangle 2
		right, bottom, 0.0f,
		right, top, 0.0f,
		left, top, 0.0f,
	};

	// UVs work top to bottom, V is reversed to get image in correct orientation
	const GLuint valuesPerCoord = 4;
	std::vector<float> tcoords = {
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f
	};

	const GLuint positionAttribId = 0;
	const GLuint texCoordAttribId = 1;

	// Generate buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &texCoordBuffer);

	// Load positions
	glEnableVertexAttribArray(positionAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glVertexAttribPointer(positionAttribId, valuesPerPosition, GL_FLOAT, false, 0, 0);
	BufferVector(GL_ARRAY_BUFFER, positions, GL_STATIC_DRAW);

	// Load UVs
	glEnableVertexAttribArray(texCoordAttribId);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glVertexAttribPointer(texCoordAttribId, valuesPerCoord, GL_FLOAT, false, 0, 0);
	BufferVector(GL_ARRAY_BUFFER, tcoords, GL_STATIC_DRAW);
}

void Quad::CreateShaders()
{
	std::string glsl_vertex = R"glsl(
		#version 330

		layout(location = 0) in vec4 vertexPosition;
		layout(location = 1) in vec4 vertexTCoord;

		out vec4 TCoord;

		void main()
		{
			gl_Position = vertexPosition;
			TCoord = vertexTCoord;
		}
	)glsl";

	std::string glsl_fragment = R"glsl(
		#version 330

		in vec4 TCoord;

		uniform sampler2D textureSampler;
		layout(location = 0) out vec4 color;

		void main() 
		{
			color = texture(textureSampler, TCoord.rg);
			//color = TCoord;
		}
	)glsl";

	auto compileAndPrintStatus = [](GLuint glShaderId) -> GLint {
		GLint compileStatus = 0;
		glCompileShader(glShaderId);
		glGetShaderiv(glShaderId, GL_COMPILE_STATUS, &compileStatus);

		if (compileStatus == GL_FALSE)
		{
			std::string message("");

			int infoLogLength = 0;
			glGetShaderiv(glShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength == 0)
			{
				message = "Message is empty (GL_INFO_LOG_LENGTH == 0)";
			}
			else
			{
				std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogLength]);
				int charsWritten = 0;
				glGetShaderInfoLog(glShaderId, infoLogLength, &charsWritten, infoLog.get());
				message = std::string(infoLog.get());
			}

			std::cout << L"GL_INFO_LOG: " << message;
		}

		return compileStatus;
	};

	// Generate program and shaders
	glProgram = glCreateProgram();
	GLint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	// Load vertex shader
	GLint sourceLength = (GLint)glsl_vertex.size();
	const char *vertexSourcePtr = glsl_vertex.c_str();
	glShaderSource(vertex_shader_id, 1, &vertexSourcePtr, &sourceLength);

	// Load fragment shader
	sourceLength = (GLint)glsl_fragment.size();
	const char *fragmentSourcePtr = glsl_fragment.c_str();
	glShaderSource(fragment_shader_id, 1, &fragmentSourcePtr, &sourceLength);

	// Compile it
	if (compileAndPrintStatus(vertex_shader_id) == GL_FALSE ||
		compileAndPrintStatus(fragment_shader_id) == GL_FALSE)
	{
		std::cout << L"Failed to compile shaders\n";
	}

	// Link it
	glAttachShader(glProgram, vertex_shader_id);
	glAttachShader(glProgram, fragment_shader_id);

	glBindAttribLocation(glProgram, 0, "vertexPosition");
	glBindAttribLocation(glProgram, 1, "vertexTCoord");

	glLinkProgram(glProgram);
	glUseProgram(glProgram);

	// Remove unlinked shaders as we don't need them any longer
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
}

