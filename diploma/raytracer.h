#pragma once

#include <math.h>
#include <windows.h>

#include <map>
#include <vector>

#define RAYTRACER_MAXSAMPLES 16

namespace raytracer{
// Vector struct for dealing with 3D vectors
struct vector{
	float x,y,z;

	vector(){}
	vector(float l, float m, float n){x=l;y=m;z=n;}

	// Addition and subtraction
	vector operator+(vector r){return vector(x + r.x, y + r.y, z + r.z);}
	vector operator-(vector r){return vector(x - r.x, y - r.y, z - r.z);}
	// Unary minus
	vector operator-(){return vector(-x, -y, -z );}
	// Multiplication and division
	vector operator*(double r){return vector(x * r, y * r, z * r);}
	vector operator/(double r){return vector(x / r, y / r, z / r);}

	// Color trickery
	vector operator*(vector r){return vector(x * r.x, y * r.y, z * r.z);}
	vector operator/(vector r){return vector(x / r.x, y / r.y, z / r.z);}


	// Vector dot product
	double operator%(vector r){return x * r.x + y * r.y + z * r.z;}
	// Cross product
	vector operator^(vector r){return vector(y * r.z - z * r.y,
											 z * r.x - x * r.z,
											 x * r.y - y * r.x);}
	// Normalizing
	vector operator!(){ return *this/sqrt(*this%*this); }
	// Get length
	float operator~(){ return sqrt(*this%*this); }
};

// Declare Renderable for traceresp use
class Renderable;

struct traceresp // Trace response
{
	traceresp(){};
	traceresp(	bool	Phit,
				vector	Phitpos		= vector(0,0,0),
				vector	Phitnormal	= vector(0,0,0),
				vector	Pnormal		= vector(0,0,0),
				bool	Pintout		= false,
				float	Plen		=-1.f,
				vector	Pcolor		= vector(0,0,0),
				float	Prefl		= 0.f,
				float	Prefr		= 0.f,
				bool	Plight		= false,
				Renderable* Pobj	= NULL)
	{
		hit			= Phit; 
		hitpos		= Phitpos; 
		hitnormal	= Phitnormal;
		normal		= Pnormal;
		intout		= Pintout;
		len			= Plen;
		color		= Pcolor;
		refl		= Prefl;
		refr		= Prefr;
		light		= Plight;
		obj			= Pobj;
	};
	
	bool	hit;			// Did we hit
	vector	hitpos;			// Where did we hit?
	vector	hitnormal;		// What's the normal to the surface?
	vector	normal;			// What's the tracer normal?

	// In fact, this is not a right approach if we want to have normal refraction
	// on the edge of 2 different materials, not just air-mat, mat-air.
	// but that'd require a stack of penetrated objects implementation, which is, frankly saying,
	// ludicrous with current architecture(Read: Full rewrite for feature of ambiguous applicability). 
	bool	intout;			// We hit the primitive from inside to outside!
	
	float	len;			// What is real length of the hit ray?
	vector	color;			// What's the color of the object hit?
	float	refl;			// Reflectivity
	float	refr;			// Refraction coefficient
	bool	light;			// Is this a point light?
	Renderable* obj;		// Pointer to hit object
};

traceresp GetIntersection(vector Or, vector Dir);
traceresp ColorRaytraceSample(vector Origin, vector Direction, int Samples = 0, float RefrIn=1.f);
//---------------------------------------------------------------
// Convenience typedefs!
//---------------------------------------------------------------
// Technically RGBA, don't forget that bmp has BGR
typedef unsigned int Pixel;
//---------------------------------------------------------------
// CanvasData class - Inspired heavily by Jacco Bikker
//---------------------------------------------------------------
class CanvasData
{
public:
// Functions
	//Ctor/Dtor
	CanvasData(int W,int H){
		Width = W; 
		Height = H;
		pixels = new Pixel[W*H];
	};
	~CanvasData()
	{
		delete [] pixels;
	};

	int GetWidth(){return Width;};
	int GetHeight(){return Height;};
	Pixel* GetPixels(){return pixels;};
	void Clear(Pixel Color)
	{
		for (int i = 0; i<Width*Height; i++)
		{
			pixels[i] = Color;
		};
	};
// Vars
	Pixel* pixels;		// Pointer to RGBA info
private:
	int Width,	Height;	// Width, Height, in pixels

};
// Render to canvas
void DrawRaytraced(CanvasData &canv);
//---------------------------------------------------------------
// Rendering classes
//---------------------------------------------------------------

// Base class
class Renderable
{
public:
// Funcs
	virtual traceresp Draw(vector Or, vector Dir) = 0; //(sic!) Infinite ray!
// Vars
	int id; // Scene id for quick reverse-lookup
	// Object parameters!
	vector pos;
	vector ang; // No strict rules as of yet. Will be clamped to -180/180.

	vector	color;	// 0-255 colors
	float	refl;	// Reflectivity
	float	refr;	// Refraction coefficient
	float	diff;	// Diffuse reflectivity coefficient - How 'granular' is our surface
	float	spec;	// Specular reflectivity coefficient - how 'well-polished' it it?

	bool	light;		// Is this a point light source?

	// TODO: Texture/material
private:

};

// The scene itself
class Scene 
{
public:
// Funcs
	traceresp Draw(vector Or, vector Dir); //(sic!) Infinite ray!
// Init function that enables some optimisation efforts!
	void Init();
// Todo: Wipe scene, Load scene, Model Precache!
// Vars
	std::map<int,Renderable*> sceneobjects;
	vector campos;
	vector camdir;
// Accel: light list
	std::vector< Renderable* > lights;	// Additional list of lights that are in sceneobjects, but since amt of lights << amt of objects...
};

// Perfect sphere
class Sphere: public Renderable
{
public:
// Funcs
	Sphere();	// Do not call this
	~Sphere();
	Sphere(vector Pos, vector Ang, vector Color, float Refl, float Refr, float Diff, float Spec, float Radius)
	{
		pos		= Pos;		ang		= Ang;
		color	= Color;	refl	= Refl;
		refr	= Refr;		diff	= Diff;		
		spec	= Spec;		radius	= Radius;
	};

	virtual traceresp Draw(vector Or, vector Dir); //(sic!) Infinite ray!
// Vars
	float	radius; // Radius
};
// Infinite plane
class Plane: public Renderable
{
public:
// Funcs
	Plane();	// Do not call this
	~Plane();
	// Pos is point, Norm is a NORMAL FROM SURFACE!
	Plane(vector Pos, vector Norm, vector Color, float Refl, float Refr, float Diff, float Spec)
	{
		pos		= Pos;		norm	= !Norm; // (sic!) optimisation!
		color	= Color;	refl	= Refl; refr	= Refr;
		diff	= Diff;		spec	= Spec;
	};

	virtual traceresp Draw(vector Or, vector Dir); //(sic!) Infinite ray!
// Vars
	vector norm;
};

// Triangle
class Triangle: public Renderable
{
public:
// Funcs
	Triangle();	// Do not call this
	~Triangle();
	// Pos is point, Norm is a NORMAL FROM SURFACE!
	Triangle(vector Pos, vector Pos1, vector Pos2, vector Color, float Refl, float Refr, float Diff, float Spec)
	{
		pos		= Pos;		pos1	= Pos1;		pos2	= Pos2;
		color	= Color;	refl	= Refl;		refr	= Refr;
		diff	= Diff;		spec	= Spec;
	};
	// Implementation of http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	virtual traceresp Draw(vector Or, vector Dir); //(sic!) Infinite ray!
// Vars
	vector pos1,pos2; // second and third vertices respectively
};


//---------------------------------------------------------------
// Current Scene
//---------------------------------------------------------------
static Scene sc;
void LoadScene(std::string file);
void InsertOBJ(std::string file, vector color, float refl, float refr, float diff, float spec, int &oindex);
void SaveRenderImage(std::string file, CanvasData &canv);
};