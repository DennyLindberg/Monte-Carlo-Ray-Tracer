/*
	Monte Carlo Ray Tracer implemented for the course TNCG15: Advanced Global Illumination and Rendering

	Copyright Denny Lindberg and Molly Middagsfjell 2018
*/

#include "datatypes.h"

class HexagonScene : public Scene
{
protected:
	PolygonObject* ceiling;
	PolygonObject* floor;
	PolygonObject* walls;

public:
	HexagonScene()
	{
		ceiling = CreateObject<PolygonObject>();
		floor = CreateObject<PolygonObject>();
		walls = CreateObject<PolygonObject>();

		ceiling->color = ColorDbl( 1.0f, 1.0f, 1.0f );
		floor->color   = ColorDbl( 1.0f, 0.0f, 0.0f );
		walls->color   = ColorDbl( 0.0f, 1.0f, 0.0f );

		// Ceiling corners
		//	   {  width, height, length }
		vec3 c1{   0.0f,   5.0f,  -3.0f };
		vec3 c2{   6.0f,   5.0f,   0.0f };
		vec3 c3{   6.0f,   5.0f,  10.0f };
		vec3 c4{   0.0f,   5.0f,  13.0f };
		vec3 c5{  -6.0f,   5.0f,  10.0f };
		vec3 c6{  -6.0f,   5.0f,   0.0f };

		// Floor corners (same, but height is flipped)
		vec3 f1 = c1; f1.y *= -1.0f;
		vec3 f2 = c2; f2.y *= -1.0f;
		vec3 f3 = c3; f3.y *= -1.0f;
		vec3 f4 = c4; f4.y *= -1.0f;
		vec3 f5 = c5; f5.y *= -1.0f;
		vec3 f6 = c6; f6.y *= -1.0f;

		floor->AddQuad(f1, f4, f3, f2);
		floor->AddQuad(f1, f6, f5, f4);

		ceiling->AddQuad(c1, c2, c3, c4);
		ceiling->AddQuad(c4, c5, c6, c1);

		walls->AddQuad(f1, f2, c2, c1);
		walls->AddQuad(f2, f3, c3, c2);
		walls->AddQuad(f3, f4, c4, c3);
		
		walls->AddQuad(f4, f5, c5, c4);
		walls->AddQuad(f5, f6, c6, c5);
		walls->AddQuad(f6, f1, c1, c6);
	}

	~HexagonScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 10.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	void AddExampleSpheres(float radius = 2.0f)
	{
		SphereObject* leftSphere = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere = CreateObject<SphereObject>();

		leftSphere->surfaceType = SurfaceType::Diffuse;
		middleSphere->surfaceType = SurfaceType::Refractive;
		rightSphere->surfaceType  = SurfaceType::Diffuse;

		leftSphere->radius = radius;
		middleSphere->radius = radius;
		rightSphere->radius = radius;

		float widthOffset = 6.0f - radius;
		float heightOffset = 5.0f - radius;
		leftSphere->position = vec3(widthOffset, -heightOffset, 10.0f);
		middleSphere->position = vec3(0.0f, -heightOffset, 8.0f);
		rightSphere->position = vec3(-widthOffset, -heightOffset, 6.0f);

		leftSphere->color = ColorDbl(1.0f, 0.0f, 0.0f);
		middleSphere->color = ColorDbl(0.0f, 1.0f, 0.0f);
		rightSphere->color = ColorDbl(0.0f, 0.0f, 1.0f);
	}

	void AddExampleLight(ColorDbl lightColor, bool usePoint = false)
	{
		vec3 roofCenter{0.0f, 5.0f-0.001f, 8.0f};

		if (usePoint)
		{
			SphereObject* pointLight = CreateObject<SphereObject>();
			pointLight->radius = 0.0f;
			pointLight->color = lightColor;
			pointLight->emission = lightColor;
			pointLight->position = roofCenter;
		}
		else
		{
			LightQuad* light = CreateObject<LightQuad>();
			light->SetGeometry(roofCenter,				// position
								{ 0.0f, -1.0f, 0.0f },	// direction
								{ 1.0f, 0.0f, 0.0f },	// side
								{ 2.0f, 2.0f });		// dimensions
			light->color = lightColor;
			light->emission = lightColor;
		}
	}
};

class CornellBoxScene : public Scene
{
protected:
	float halfLength = 0.0f;
	float halfWidth = 0.0f;
	float halfHeight = 0.0f;

	PolygonObject* leftWall;
	PolygonObject* rightWall;
	PolygonObject* whiteSegments;

public:
	CornellBoxScene(float length, float width, float height)
	{
		halfLength = length / 2.0f;
		halfWidth = width / 2.0f;
		halfHeight = height / 2.0f;

		/*
			See room definition at 
				https://en.wikipedia.org/wiki/Cornell_box
		*/
		leftWall = CreateObject<PolygonObject>();
		rightWall = CreateObject<PolygonObject>();
		whiteSegments = CreateObject<PolygonObject>();

		leftWall->color		 = ColorDbl( 1.0f, 0.0f, 0.0f );
		rightWall->color	 = ColorDbl( 0.0f, 1.0f, 0.0f );
		whiteSegments->color = ColorDbl( 1.0f, 1.0f, 1.0f );

		// Box corners
		vec3 c1{ -halfWidth, halfHeight,  halfLength };
		vec3 c2{  halfWidth, halfHeight,  halfLength };
		vec3 c3{  halfWidth, halfHeight, -halfLength };
		vec3 c4{ -halfWidth, halfHeight, -halfLength };

		// Floor corners (same, but height is flipped)
		vec3 f1 = c1; f1.y *= -1.0f;
		vec3 f2 = c2; f2.y *= -1.0f;
		vec3 f3 = c3; f3.y *= -1.0f;
		vec3 f4 = c4; f4.y *= -1.0f;

		leftWall->AddQuad(f2, c2, c3, f3);
		rightWall->AddQuad(f1, f4, c4, c1);

		whiteSegments->AddQuad(c1, c4, c3, c2); // Ceiling
		whiteSegments->AddQuad(f4, f3, c3, c4);	// Back wall
		whiteSegments->AddQuad(f1, f2, f3, f4);	// Floor
	}

	~CornellBoxScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 1.5f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	void AddExampleSpheres(float radius = 0.25f)
	{
		SphereObject* leftSphere   = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere  = CreateObject<SphereObject>();

		leftSphere->surfaceType   = SurfaceType::Diffuse;
		middleSphere->surfaceType = SurfaceType::Specular;
		rightSphere->surfaceType  = SurfaceType::Diffuse;

		leftSphere->radius   = radius;
		middleSphere->radius = radius;
		rightSphere->radius  = radius;

		float widthOffset  = halfWidth - radius;
		float depthOffset  = halfLength - radius;
		float heightOffset = halfHeight - radius;
		leftSphere->position   = vec3(-widthOffset,	-heightOffset, -depthOffset);
		middleSphere->position = vec3(0, 0, 0);
		rightSphere->position  = vec3(widthOffset, heightOffset, depthOffset);

		leftSphere->color   = ColorDbl(1.0f, 0.0f, 0.0f);
		middleSphere->color = ColorDbl(0.0f, 1.0f, 0.0f);
		rightSphere->color  = ColorDbl(0.0f, 0.0f, 1.0f);
	}

	void AddExampleLight(ColorDbl lightColor, bool usePoint = false)
	{
		vec3 roofCenter{0.0f, halfHeight - 0.001f, 0.0f};

		if (usePoint)
		{
			SphereObject* pointLight = CreateObject<SphereObject>();
			pointLight->radius = 0.0f;
			pointLight->color = lightColor;
			pointLight->emission = lightColor;
			pointLight->position = roofCenter;
		}
		else
		{
			LightQuad* light = CreateObject<LightQuad>();
			light->SetGeometry( roofCenter,									// position
								{ 0.0f, -1.0f, 0.0f },						// direction
								{ 1.0f, 0.0f, 0.0f },						// side
								{ halfWidth / 2.0f, halfHeight / 2.0f });	// dimensions
			light->color = lightColor;
			light->emission = lightColor;
		}
	}
};
