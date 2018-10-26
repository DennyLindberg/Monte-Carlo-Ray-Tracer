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
	PolygonObject* walls1;
	PolygonObject* walls2;
	PolygonObject* walls3;

public:
	HexagonScene()
	{
		ceiling = CreateObject<PolygonObject>();
		floor = CreateObject<PolygonObject>();
		walls1 = CreateObject<PolygonObject>();
		walls2 = CreateObject<PolygonObject>();
		walls3 = CreateObject<PolygonObject>();

		ceiling->color = ColorDbl( 0.2 );
		floor->color   = ColorDbl( 0.2 );
		walls1->color = ColorDbl(0.2, 0.01, 0.01);
		walls2->color = ColorDbl(0.01, 0.2, 0.01);
		walls3->color = ColorDbl(0.2);

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

		walls1->AddQuad(f2, f3, c3, c2);
		walls2->AddQuad(f1, f2, c2, c1);
		walls3->AddQuad(f3, f4, c4, c3);
		
		walls2->AddQuad(f5, f6, c6, c5);
		walls3->AddQuad(f4, f5, c5, c4);
		walls3->AddQuad(f6, f1, c1, c6);
	}

	~HexagonScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 10.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	void AddExampleSpheres(float radius = 1.5f)
	{
		SphereObject* leftSphere = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere = CreateObject<SphereObject>();

		leftSphere->surfaceType = SurfaceType::Diffuse;
		middleSphere->surfaceType = SurfaceType::Specular;
		rightSphere->surfaceType  = SurfaceType::Diffuse;

		leftSphere->radius = radius;
		middleSphere->radius = radius;
		rightSphere->radius = radius;

		float widthOffset = 6.0f - radius;
		float heightOffset = 5.0f - radius;
		leftSphere->position = vec3(3.0, 2.0, 10.0f);
		middleSphere->position = vec3(-3.0f, 0.0, 8.0f);
		rightSphere->position = vec3(1.0f, -3.0, 6.0f);

		leftSphere->color = ColorDbl(0.5);
		middleSphere->color = ColorDbl(0.5);
		rightSphere->color = ColorDbl(0.5);

		// Cube1
		CubeObject* cube = CreateObject<CubeObject>();
		cube->SetGeometry({ 3.0, -5.0, 10.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, 7.0f-radius);
		cube->color = ColorDbl(0.01, 0.3, 0.8);
		cube->surfaceType = SurfaceType::Diffuse;

		// Cube2
		CubeObject* cube2 = CreateObject<CubeObject>();
		cube2->SetGeometry({ -3.0, -5.0, 8.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, 5.0f-radius);
		cube2->color = ColorDbl(0.8, 0.4, 0.01);
		cube2->surfaceType = SurfaceType::Diffuse;

		// Cube2 - middle
		CubeObject* cube3 = CreateObject<CubeObject>();
		cube3->SetGeometry({ 1.0, -5.0, 6.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 4.0f, 4.0f, 2.0f - radius);
		cube3->color = ColorDbl(0.5, 0.2, 0.8);
		cube3->surfaceType = SurfaceType::Diffuse;
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
								{ 1.0f, 1.0f });		// dimensions
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

		leftWall->color		 = ColorDbl( 0.2, 0.01, 0.01 );
		rightWall->color	 = ColorDbl( 0.01, 0.2, 0.01 );
		whiteSegments->color = ColorDbl( 0.2 );

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

		whiteSegments->AddQuad(c4, c3, c2, c1); // Ceiling
		whiteSegments->AddQuad(f4, f3, c3, c4);	// Back wall
		whiteSegments->AddQuad(f1, f2, f3, f4);	// Floor
	}

	~CornellBoxScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, halfLength), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	void AddExampleSpheres(float radius = 1.5f)
	{
		SphereObject* leftSphere   = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere = CreateObject<SphereObject>();
		SphereObject* airSphere  = CreateObject<SphereObject>();

		leftSphere->surfaceType   = SurfaceType::Diffuse;
		middleSphere->surfaceType = SurfaceType::Specular;
		rightSphere->surfaceType = SurfaceType::Diffuse;
		airSphere->surfaceType  = SurfaceType::Diffuse;

		leftSphere->radius   = radius;
		middleSphere->radius = radius*1.5f;
		rightSphere->radius = radius;
		airSphere->radius  = radius;

		float widthOffset  = halfWidth - radius;
		float depthOffset  = halfLength - radius;
		float heightOffset = halfHeight - radius;
		leftSphere->position   = vec3(-widthOffset,	-heightOffset, 0.0f);
		middleSphere->position = vec3(0, -halfHeight+middleSphere->radius, -halfLength+middleSphere->radius);
		rightSphere->position = vec3(widthOffset, 0.0f, -depthOffset / 2.0);
		airSphere->position  = vec3(0, heightOffset, -depthOffset);

		leftSphere->color   = ColorDbl(0.5);
		middleSphere->color = ColorDbl(0.5);
		rightSphere->color = ColorDbl(0.5);
		airSphere->color  = ColorDbl(0.5);

		// Cube1
		CubeObject* cube = CreateObject<CubeObject>();
		cube->SetGeometry({ halfWidth-1.5f, -halfHeight, -depthOffset/2.0 }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 2.0f, 2.0f, halfHeight-radius);
		cube->color = ColorDbl(0.01, 0.3, 0.8);
		cube->surfaceType = SurfaceType::Diffuse;

		// Cube2
		CubeObject* cube2 = CreateObject<CubeObject>();
		cube2->SetGeometry({ -halfWidth + 1.5f, -halfHeight, -depthOffset + 1.5f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.0f, 1.0f }, 1.5f, 1.5f, halfHeight+radius);
		cube2->color = ColorDbl(0.8, 0.4, 0.01);
		cube2->surfaceType = SurfaceType::Diffuse;
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
								{ halfWidth / 3.0f, halfHeight / 3.0f });	// dimensions
			light->color = lightColor;
			light->emission = lightColor;
		}
	}
};
