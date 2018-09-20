/*
	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "screenshot.h"

#include <iostream>
#include <fstream>

#include "glad/glad.h"
#include "../thirdparty/lodepng.h"

void TakeScreenshot(std::string filename, unsigned int screenWidth, unsigned int screenHeight)
{
	/*
		Determine file output
	*/
	auto remove_extension = [](const std::string& filename) -> std::string {
		size_t lastdot = filename.find_last_of(".");
		if (lastdot == std::string::npos) return filename;
		return filename.substr(0, lastdot);
	};

	filename = remove_extension(filename);

	auto file_exists = [](std::string filename) -> bool {
		std::ifstream infile(filename);
		return infile.good();
	};

	int count = 0;
	std::string baseName = filename;
	do
	{
		count++;
		filename = baseName + std::string(5 - std::to_string(count).length(), '0') + std::to_string(count) + ".png";
	} while (file_exists(filename));

	/*
		Get data from framebuffer
	*/
	std::vector<GLubyte> data(4 * screenWidth * screenHeight);
	glReadPixels(0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	/*
		Write PNG
	*/
	unsigned error = lodepng::encode(filename, data, screenWidth, screenHeight);
	if (error)
	{
		std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}
}
