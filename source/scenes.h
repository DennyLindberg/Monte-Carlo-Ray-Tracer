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

		ceiling->color = vec4{ 0.5f, 0.5f, 1.0f, 1.0f };
		floor->color = vec4{ 0.2f, 0.2f, 0.2f, 1.0f };
		walls->color = vec4{ 0.5f, 0.2f, 0.3f, 1.0f };

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

		floor->AddQuad(f1, f2, f3, f4);
		floor->AddQuad(f4, f5, f6, f1);

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

	void AddExampleSpheres(float radius = 2.0f)
	{
		SphereObject* leftSphere = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere = CreateObject<SphereObject>();

		leftSphere->radius = radius;
		middleSphere->radius = radius;
		rightSphere->radius = radius;

		leftSphere->transform.position   = vec4(-radius*2,	-5.0f + radius, 8.0 + radius, 1.0);
		middleSphere->transform.position = vec4(0.0,		-5.0f + radius, 8.0,		  1.0);
		rightSphere->transform.position  = vec4(radius*2,	-5.0f + radius, 8.0 - radius, 1.0);

		leftSphere->color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		middleSphere->color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		rightSphere->color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
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

		leftWall->color		 = vec4{ 1.0f, 0.0f, 0.0f, 1.0f };
		rightWall->color	 = vec4{ 0.0f, 1.0f, 0.0f, 1.0f };
		whiteSegments->color = vec4{ 1.0f };

		// Box corners
		vec3 c1{ -halfWidth, halfHeight, -halfLength };
		vec3 c2{  halfWidth, halfHeight, -halfLength };
		vec3 c3{  halfWidth, halfHeight,  halfLength };
		vec3 c4{ -halfWidth, halfHeight,  halfLength };

		// Floor corners (same, but height is flipped)
		vec3 f1 = c1; f1.y *= -1.0f;
		vec3 f2 = c2; f2.y *= -1.0f;
		vec3 f3 = c3; f3.y *= -1.0f;
		vec3 f4 = c4; f4.y *= -1.0f;

		leftWall->AddQuad(f1, f4, c4, c1);
		rightWall->AddQuad(f2, c2, c3, f3);

		whiteSegments->AddQuad(f1, f2, f3, f4);
		whiteSegments->AddQuad(c1, c2, c3, c4);
		whiteSegments->AddQuad(f3, c3, c4, f4);
	}

	~CornellBoxScene() = default;

	virtual void MoveCameraToRecommendedPosition(Camera& camera)
	{
		camera.SetView(vec3(0.0f, 0.0f, -2.5f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	void AddExampleSpheres(float radius = 0.25f)
	{
		SphereObject* leftSphere   = CreateObject<SphereObject>();
		SphereObject* middleSphere = CreateObject<SphereObject>();
		SphereObject* rightSphere  = CreateObject<SphereObject>();

		leftSphere->radius   = radius;
		middleSphere->radius = radius;
		rightSphere->radius  = radius;

		leftSphere->transform.position   = vec4(-radius * 2, -halfHeight + radius,	radius,  1.0);
		middleSphere->transform.position = vec4(0.0,		 -halfHeight + radius,	0.0,	 1.0);
		rightSphere->transform.position  = vec4(radius * 2,	 -halfHeight + radius,	-radius, 1.0);

		leftSphere->color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		middleSphere->color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		rightSphere->color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	}
};