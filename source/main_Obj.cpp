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
#include <vector>

#include "core.h"
#include "imageconversion.h"

static const bool SCREEN_VSYNC = false;
static const int SCREEN_FULLSCREEN = 0;
static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;

static const float SCREEN_UPDATE_DELAY = 2.0f;
static const bool USE_MULTITHREADING = true;

int main()
{
    //OBJ Loader
    objl::Loader load;
    bool loadTrue = load.LoadFile("box_stack.obj");
    
    if(loadTrue)
    {
        std::ofstream file("printOut.txt");
        for (int i = 0; i < load.LoadedMeshes.size(); i++)
        {
            //Copy Mesh
            objl::Mesh copyMesh = load.LoadedMeshes[i];
            
            //Mesh Name into file
            file << "MESH: "  << i << copyMesh.MeshName << std::endl;
            
            //Vertices into file.
            file << "Vertices" << std::endl;
            
            for(int j = 0; j < copyMesh.Vertices.size(); j++)
            {
                file << "Vertice: " << j << std::endl;
                file << "Position: " << "( " << copyMesh.Vertices[j].Position.X << ", " << copyMesh.Vertices[j].Position.Y << ", " << copyMesh.Vertices[j].Position.Z << ")" << std::endl;
                file << "Normal: " << "( " << copyMesh.Vertices[j].Normal.X << ", " << copyMesh.Vertices[j].Normal.Y << ", " << copyMesh.Vertices[j].Normal.Z << ")" << std::endl;
                file << "Texture Coordinates: " << "(" << copyMesh.Vertices[j].TextureCoordinate.X << ", " << copyMesh.Vertices[j].TextureCoordinate.Y << ")" << std::endl;
            }
            
            //Indices into file
            file << "Indices: "<< std::endl;
            
            //Triangle
            for(int k = 0; k < copyMesh.Indices.size(); k++)
            {
                //Triangle requires 3 indices for one triangle
                if(k%3 == 0)
                {
                    file << "Triangle: " << k << std::endl;
                    file << "(" << copyMesh.Indices[k] << ", " << copyMesh.Indices[k+1] << ", " << copyMesh.Indices[k+2] << ")"<< std::endl;
                }
            }
    
        }
        
        file.close();
    }
        
    else{
        
        std::ofstream file("printOut.txt");
        std::cout << file.good() << std::endl;
        file.close();
    }
    
    
    
    //std::cout << load.LoadedMeshes[0].MeshName << std::endl;
    
	OpenGLWindow window("OpenGL", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	window.SetClearColor(0.0, 0.0, 0.0, 1.0f);

	GLFullscreenImage glImage(SCREEN_WIDTH, SCREEN_HEIGHT, 4);
	PixelBuffer pixels(SCREEN_WIDTH, SCREEN_HEIGHT, 4);

	/*
		Variables used for filling the screen with lines
	*/
	unsigned int pixelId = 0;
	double r = 1000.0;
	double g = 0.0;
	double b = 0.0;

	/*
		Application loop
	*/
	ApplicationClock clock;
	float lastScreenUpdate = clock.Time();
	bool quit = false;
	while (!quit) 
	{
		clock.Tick();
		window.SetTitle("FPS: " + std::to_string(1 / clock.DeltaTime()) + " - Time: " + std::to_string(clock.Time()));
		
		/*
			Fill screen with diagonal lines
		*/
		pixelId += 3*12;
		if ((int) pixelId >= pixels.size())
		{
			pixelId = 0;

			// Alternate between red and green when screen has been filled
			if (r == 1000.0) { r = 0.0; g = 1000.0; b = 0.0; }
			else			 { r = 1000.0; g = 0.0; b = 0.0; }
		}
		pixels[pixelId] = r; pixels[pixelId+1] = g; pixels[pixelId+2] = b; pixels[pixelId+3] = 1000.0;

		/*
			Update image on screen every SCREEN_UPDATE_DELAY seconds
		*/
		float timeSinceLastUpdate = clock.Time() - lastScreenUpdate;
		if (timeSinceLastUpdate >= SCREEN_UPDATE_DELAY)
		{
			lastScreenUpdate = clock.Time();

			// Redraw screen with updated image
			CopyPixelsToImage(pixels, glImage, USE_MULTITHREADING);
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