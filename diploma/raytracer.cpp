#include "stdafx.h"

#include "raytracer.h"
// headers needed for .obj reading
#include <vector>
#include <sstream>
#include <fstream>

using namespace raytracer;
#define EPSILON 0.000001
//---------------------------------------------------------------
// Sphere
//---------------------------------------------------------------
traceresp Sphere::Draw(vector Or, vector Dir)
{
	vector stp = Or;
	vector enp = Or+Dir*1e6;
	vector sphp = pos;
	double Dist;

	double a = (enp.x-stp.x)*(enp.x-stp.x)+(enp.y-stp.y)*(enp.y-stp.y)+(enp.z-stp.z)*(enp.z-stp.z); // Usable also for non-infinite rays
	double b = 2*((enp.x-stp.x)*(stp.x-sphp.x)+(enp.y-stp.y)*(stp.y-sphp.y)+(enp.z-stp.z)*(stp.z-sphp.z));
	double c = (stp.x-sphp.x)*(stp.x-sphp.x) + (stp.y-sphp.y)*(stp.y-sphp.y) + (stp.z-sphp.z)*(stp.z-sphp.z) - radius*radius;

	double D = b*b - 4*a*c;
	if (D<0) {
		return traceresp(false);
	}

	if(D==0)
		Dist = -b/(2*a);
	if(D>0)
	{
		double d1 = (-b-sqrt(D))/(2*a);
		double d2 = (-b+sqrt(D))/(2*a);
		Dist = min(d1,d2);
	}

	if(Dist>=0.f){
		traceresp result(false);
		result.hit			= true;
		result.normal		= Dir;
		result.hitpos		= stp+(enp-stp)*Dist;// Assuming dir normalized
		result.hitnormal	= !(result.hitpos-pos);
		result.len			= Dist;
		result.color		= color;
		result.refl			= refl;
		result.refr			= refr;
		result.light		= light;
		result.obj			= this;
		// If ray started inside sphere, then set intout to true
		// Squared distance check is a little bit quicker than sqrt, also shave call/stack manip time
		if ((Or.x-sphp.x)*(Or.x-sphp.x) + (Or.y-sphp.y)*(Or.y-sphp.y) + (Or.z-sphp.z)*(Or.z-sphp.z) < radius*radius) 
			result.intout = true;

		return result;
	}
	else
	{
		return traceresp(false);
	}
};
//---------------------------------------------------------------
// Plain ol' Plane
//---------------------------------------------------------------
traceresp Plane::Draw(vector Or, vector Dir)
{
	double denom = norm%Dir;
	if (abs(denom) > EPSILON) // Check it so we don't have division by zero cases, these happen when ray is parallel to plane
	{
		double t = (pos - Or)%(norm) / denom; // Double to prevent rounding errors bonanza
		if (t >= 0) 
		{
			traceresp result(false);
			result.hit			= true;
			result.normal		= Dir;
			result.hitpos		= Or+(Dir)*t;
			result.hitnormal	= norm;
			result.len			= t;//*1e6;
			result.color		= color;
			result.refl			= refl;
			result.refr			= refr;
			result.light		= light;
			result.obj			= this;

			if (denom>0)
				result.intout = true;
			
			return result;
		}; 
	};

	return traceresp(false);
};

//---------------------------------------------------------------
// Triangle
//---------------------------------------------------------------
// Implementation of Moller-Trumbore intersection algorithm
traceresp Triangle::Draw(vector Or, vector Dir)
{
	vector dir(Dir);
	Dir = Dir*1e3;	// Strangely enough, this produces cleaner results
					// digging documentation yielded nothing?..
					// Likely to be another case of floating point rounding errors

	vector e1, e2;  //Edge1, Edge2	
	vector P, Q, T;
	double det, inv_det, u, v;
	double t;
 
	//Find vectors for two edges sharing V1
	e1 = pos1-pos;
	e2 = pos2-pos;

	//Begin calculating determinant - also used to calculate u parameter
	P = Dir^e2;
	//if determinant is near zero, ray lies in plane of triangle
	det = e1%P;
	// Culling
	//if (det<EPSILON)
	//NOT CULLING
	if(det > -EPSILON && det < EPSILON) return traceresp(false);
	
	inv_det = 1.f / det;
 
	//calculate distance from V1 to ray origin
	T = Or-pos;
 
	//Calculate u parameter and test bound
	u = (T%P)*inv_det;
	//The intersection lies outside of the triangle
	if(u < 0.f || u > 1.f) return traceresp(false);
 
	//Prepare to test v parameter
	Q = T^e1;
 
	//Calculate V parameter and test bound
	v = (Dir%Q)*inv_det;
	//The intersection lies outside of the triangle
	if(v < 0.f || u + v  > 1.f) return traceresp(false);
 
	t = (e2%Q)*inv_det;
 
	if(t > EPSILON) { //ray intersection
		// This algorithm finds t, which is supposed multiplier of (sic!)Dir
		// that makes it touch surface.
		traceresp result(false);
		result.hit			= true;
		result.normal		= dir;
		result.hitpos		= Or+(Dir)*t;
		result.hitnormal	= (((e1^e2))%Dir)<0?!(e1^e2):!(e2^e1);
		result.len			= t*~Dir;//*1e6;
		result.color		= color; // TODO: Half-Lambertian!
		result.refl			= refl;
		result.refr			= refr;
		result.light		= light;
		result.obj			= this;

		return result;
	}
 
	// No hit, no win
	return traceresp(false);
};


//---------------------------------------------------------------
// Scene
//---------------------------------------------------------------
void Scene::Init()
{
	for (std::map<int,Renderable*>::iterator ri = sc.sceneobjects.begin(); ri!=sc.sceneobjects.end() ; ri++)
	{
		if(ri->second->light)
		{
			sc.lights.push_back(ri->second); // Add all lights into the acceleration list
		}
	}
}
traceresp Scene::Draw(vector Or, vector Dir)
{
	// We're going to iterate through ALL objects,
	// Pick best(closest to origin)
	traceresp	BestR(false);	// Best Response
	traceresp	CurR(false);	// Current response
	float		BestD = 1e9;	// Best Distance(So we don't calculate it every check!)
	float		CurD;			// Current distance, so we avoid calculating it twice!
	for (std::map<int,Renderable*>::iterator it = sceneobjects.begin(); it!=sceneobjects.end(); it++)
	{
		// TODO: Broadphase optimisation: BBOX, Dot Product
		CurR = it->second->Draw(Or,Dir);
		
		if (CurR.hit)
		{
			CurD = ~(CurR.hitpos-Or);
			if(CurD<BestD)
			{
				BestD = CurD;
				BestR = CurR;
			}
		}
	}
	return BestR;
};

traceresp raytracer::GetIntersection(vector Or, vector Dir)
{
	traceresp result(false);

	result = sc.Draw(Or,Dir);
	if (result.hit){
		return result;
	}

	if (abs(Dir.z) !=0)//;//> 0.000001) // no division by zero
	{	
		float p = -Or.z/Dir.z; // How fast do we reach floor?

		if (p > 0.01)
		{
			result.hit			= true;
			result.hitnormal	= vector(0,0,1);
			result.normal		= Dir;
			result.hitpos		= Or+Dir*p;// Assuming dir normalized
			result.len			= p;
			result.color		= (int(ceil(result.hitpos.x/5) + ceil(result.hitpos.y/5))&1?vector(0,75,100):vector(200,150,0));	// Tiled floor!
			result.refl			= 0.5f;
			result.refr			= 0.f;
			result.light		= false;
			result.obj			= NULL;

			return result;
		}
	}
	// Else - 'Hit' the sky!
	result.hit			= false; //?
	result.color		= vector(0,255,90)*pow(1-min(1.f,max(Dir.z,0)),4.f);

	return result;
}

traceresp raytracer::ColorRaytraceSample(vector Origin, vector Direction, int Samples, float RefrIn) // Handles recursive raytracing
{
	vector	rcolor = vector(0,0,0);	// Base color
	int		rsmplc = 1;				// Sample counter
	vector  lcolor = vector(0,0,0);	// Light Color

	Direction = !Direction; // ! normalized !

	traceresp rez = GetIntersection(Origin, Direction);
	if (rez.hit&&!rez.light){
		// Light system
		for(std::vector<Renderable*>::size_type i = 0; i != sc.lights.size(); i++) {
			// Check if we can see this light
			traceresp lighttest = GetIntersection(rez.hitpos+rez.hitnormal*EPSILON*100, !(sc.lights[i]->pos-(rez.hitpos+rez.hitnormal*EPSILON*100)));
			if (lighttest.hit&&lighttest.obj!=NULL&&lighttest.obj==sc.lights[i])
			{
				float odiff	= 0.0f; // Floor hack
				float ospec	= 0.0f;
				
				if(rez.obj!=NULL){
					odiff	= rez.obj->diff;
					ospec	= rez.obj->spec;
				}
				double dot = rez.hitnormal%!(sc.lights[i]->pos-rez.hitpos);
				// Calculate diffuse light
				if (odiff>0.f)
				{	// Apply diffuse light to surface
					//_asm{nop}; // Dark magic related to recompiling, use sparingly and only when sure that you know what you are doing!
					if(dot>0){
						lcolor =lcolor + (lighttest.color)/255
						*odiff
						*dot;
					}
				}
				// Calculate specular light

				if (ospec>0.f)
				{	// Apply diffuse light tint. Notice - no original color!
					// Reflected light
					vector refll = !(rez.hitpos-sc.lights[i]->pos);
					refll = !(rez.hitnormal*-2*(rez.hitnormal%refll)+refll);
					dot = (-Direction)%!refll;

					if(dot>0.f) lcolor = lcolor + (lighttest.color)/255*ospec*powf(dot,5); // Phong explonent was 20
				}
			}
		}
		lcolor = lcolor * rez.color;

		// Work with reflections
		if (rez.hit&&rez.refl>0&&Samples<RAYTRACER_MAXSAMPLES) {
			// Pre-light

			rcolor=			rcolor + lcolor*(1-rez.refl) + 

							ColorRaytraceSample(	rez.hitpos+rez.hitnormal*EPSILON*100, // Precision errors ahoy!

							!(rez.hitnormal*-2*(rez.hitnormal%rez.normal)+rez.normal),

							Samples+1,RefrIn).color * rez.refl;
		}
		else
		{
			rcolor = lcolor;
		}
		// Work with REFRACTIONS!
		if (rez.hit&&rez.refr>0&&Samples<RAYTRACER_MAXSAMPLES) {
			//rcolor = vector(0,255,0);
			float refrc = RefrIn/rez.refr;
			vector norm = rez.hitnormal;
			if (rez.intout) // hit from inside!
				norm = -norm;
			// Put Snell's law to the action!
			float cosint	= -(norm%Direction);
			float cosrefr	= 1.f - refrc*refrc*(1.f-cosint*cosint); // Calculate cosine of angle in which we must bounce off
			if (cosrefr>0.0f) // Total internal reflection avoided!
			{
				// Oh man... Without Jacco Bikker's example, this would've been a mess!
				vector ndir = (Direction*refrc)+norm*(refrc*cosint-sqrt(cosrefr));
				// Even better, Beer law is now also in effect!
				// Magic number EPSILON*100 is handpicked to remove noise related to rounding errors
				traceresp refrrez = ColorRaytraceSample(rez.hitpos+ndir*EPSILON*100,ndir,Samples+1,refrc);
				vector btr(1.f,1.f,1.f);
				if (rez.intout){
					vector bla = (rez.obj->color/255.f)*0.15f*refrrez.len;// Beer's law absorbance
					btr = vector(	expf(bla.x),
									expf(bla.y),
									expf(bla.z));
				}
				rcolor = rcolor + refrrez.color*btr;
			}
		}
	}
	else
	{
		rcolor = rez.color;
	}
	// Clamp color, as light may easily go outta bounds
	rcolor.x = floor(rcolor.x+0.5);
	rcolor.y = floor(rcolor.y+0.5);
	rcolor.z = floor(rcolor.z+0.5);

	if (rcolor.x>255) rcolor.x=255;
	if (rcolor.y>255) rcolor.y=255;
	if (rcolor.z>255) rcolor.z=255;

	if (rcolor.x<0) rcolor.x=0;
	if (rcolor.y<0) rcolor.y=0;
	if (rcolor.z<0) rcolor.z=0;

	rez.color = rcolor;

	return rez;
}


void raytracer::DrawRaytraced(CanvasData &canv)
{
	int w = canv.GetWidth();
	int h = canv.GetHeight();
	// Camera stuffs
	vector CamPos	= sc.campos;
	vector CamDir	= sc.camdir;

	vector CamRight	= -!(vector(0,0,1)^CamDir)/w; // Getting step for frustrum's down side
	vector CamUp	= !(CamDir^(-CamRight))/h; // Getting step for frustrum's left side

	// technically we could define our fov here by moving scan grid we just set up
	// certain distance away from camerapoint
	vector LowLeftCorner	= (-CamRight*w)/2 // We want to step LEFT!
							+ (CamUp*h)	 /2 
							+ CamDir; 

    // For every "pixel"
    for (int i=0; i < w * h; ++i) {

		vector Color(0,0,0);
		// Naive supersampling antialiasing. Could be optimized with edge detection, but will mess with gradients otherwise!
		Color = Color + ColorRaytraceSample(CamPos, LowLeftCorner + CamRight*(i%w) + CamUp*(-floor(i/float(w)))).color;
		Color = Color + ColorRaytraceSample(CamPos, LowLeftCorner + CamRight*((i%w)-.1) + CamUp*(-floor(i/float(w))-.1)).color;
		Color = Color + ColorRaytraceSample(CamPos, LowLeftCorner + CamRight*((i%w)-.1) + CamUp*(-floor(i/float(w))+.1)).color;
		Color = Color + ColorRaytraceSample(CamPos, LowLeftCorner + CamRight*((i%w)+.1) + CamUp*(-floor(i/float(w))-.1)).color;
		Color = Color + ColorRaytraceSample(CamPos, LowLeftCorner + CamRight*((i%w)+.1) + CamUp*(-floor(i/float(w))+.1)).color;
		Color = Color/5.f;

		canv.pixels[i] = (int(Color.x) << 16) + (int(Color.y) << 8) + int(Color.z);
    }
}

// Load .scene file
// Format:
// obj r g b refl refr diff spec PATH/FILENAME - loads triangles from obj file
// t x y z x1 y1 z1 x2 y2 z2 r g b refl refr diff spec - Creates triangle
// s x y z r g b refl refr diff spec radius	[light] - sphere, [light] may be anything, if found, mark this sphere as point light source
// c x y z dirx diry dirz - camera
// p x y z dirx diry dirz r g b refl refr diff spec - plane
void raytracer::LoadScene(std::string file)
{
	if(file.empty()||file.compare(std::string(""))==0){
		MessageBox(NULL,  L"No scene filename specified!", L"Scene Loading Failed", MB_OK|MB_ICONWARNING);
		return;
	}
	// Wipe scene and acceleration structures, in case we have something loaded
	for(std::map<int,Renderable*>::iterator sci = sc.sceneobjects.begin(); sci != sc.sceneobjects.end(); ++sci)
	{
		delete (*sci).second;	// Nuke all objects
	}
	sc.sceneobjects.clear();	// And forget about them!
	sc.lights.clear();			// That technically should invalidate pointers too

	std::ifstream scenefile (file);
	if (scenefile.is_open()){
		int oindex = 0;
		// Attempting better code practice...
		for(std::string line; std::getline(scenefile, line); )   //read stream line by line
		{
			std::istringstream in(line);      //make a stream for the line itself

			std::string type;
			in >> type;                  //and read the first whitespace-separated token

			if(type == "obj")       //and check its value
			{
			    float r, g, b, refl, refr, diff, spec;
			    in >> r >> g >> b >> refl >> refr >> diff >> spec;       //now read the whitespace-separated floats

				std::string objfilename = in.str().substr((unsigned int)in.tellg()+1); 
				// Get the string from here and to end of line. WARNING - THIS GETS FIRST SPACE SO WE ADD 1, BUT WE MAY NOT HAVE NAME AT ALL!

				InsertOBJ( objfilename, vector(r,g,b), refl, refr, diff, spec, oindex);
			}
			else 
			if (type == "t")
			{
				float x, y, z, x1, y1, z1, x2, y2, z2, r, g, b, refl, refr, diff, spec;
			    in >> x >> y >> z >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> r >> g >> b >> refl >> refr >> diff >> spec;       //now read the whitespace-separated floats

				sc.sceneobjects[oindex] = new Triangle(vector(x,y,z),vector(x1,y1,z1),vector(x2,y2,z2),vector(r, g, b),refl,refr,diff,spec);
				sc.sceneobjects[oindex]->light = false;
				oindex++;
			}
			else
			if (type == "s")
			{
				float x, y, z, r, g, b, refl, refr, diff, spec, radius;
			    in >> x >> y >> z >> r >> g >> b >> refl >> refr >> diff >> spec >> radius;       //now read the whitespace-separated floats

				bool light = (in.peek()==EOF)?false:true;  // peek character

				sc.sceneobjects[oindex] = new Sphere(vector(x,y,z),vector(0,0,0),vector(r,g,b),refl,refr,diff,spec,radius);
				sc.sceneobjects[oindex]->light = light;
				oindex++;
			}
			else
			if (type == "c")
			{
				float x, y, z, dx, dy, dz;
			    in >> x >> y >> z >> dx >> dy >> dz;       //now read the whitespace-separated floats

				sc.campos = vector(x,y,z);
				sc.camdir = !vector(dx,dy,dz);
				// Camera is not a real element, so do not increment oindex here!
			}
			else
			if (type == "p")
			{
				float x, y, z, nx, ny, nz, r, g, b, refl, refr, diff, spec;
			    in >> x >> y >> z >> nx >> ny >> nz >> r >> g >> b >> refl >> refr >> diff >> spec;       //now read the whitespace-separated floats

				sc.sceneobjects[oindex] = new Plane(vector(x,y,z),vector(nx,ny,nz),vector(r,g,b),refl,refr,diff,spec);
				sc.sceneobjects[oindex]->light = false;

				oindex++;
			}
			else
			{
				std::wstring err = L"Error during .scene file parsing. Unknown token:\"";
				err+=std::wstring(type.begin(),type.end()); // Avoid using printf with something that user can mess around with!
				err+=L"\"";
				MessageBox(NULL, err.c_str(), L".scene file failed to parse!", MB_OK|MB_ICONWARNING);
				return;
			}
		}
	}
	else
	{
		// We can fail to load the file, which is unlikely actually
		std::wstring err = L"Raytracer engine has failed to load scene!\nUnable to find or open the file: \"";
		err+=std::wstring(file.begin(),file.end()); // Avoid using printf with something that user can mess around with!
		err+=L"\"";
		MessageBox(NULL, err.c_str(), L"Scene Loading Failed", MB_OK|MB_ICONWARNING);
		return;
	}
	sc.Init();
}

void raytracer::InsertOBJ(std::string file, vector color, float refl, float refr, float diff, float spec, int &oindex)
{
	if(file.empty()||file.compare(std::string(""))==0){
		MessageBox(NULL,  L"No .obj filename specified!", L"Obj Loading Failed", MB_OK|MB_ICONWARNING);
		return;
	}
	// Load all of that .obj goodness
	// .obj parser
	// Mtl IS NOT SUPPORTED due to architectural design decision(no per-color reflectivity, etc...)
	// Technically std strings have somewhat 2GB limit on x86 systems, IIRC, so don't shove crazy things inside
	std::string str; // File we're going to parse
						
	FILE *fp;							// File object
	// This throws warning C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
	// But in fact we can't exactly reap benefits of fopen_s(bounds-checked version of fopen) version here, due to all parameters being either in-line or function parameters(Read: No nullpointer no matter what)
	// And it checks for nullpointer runtime, so that would be slowing code down
    fp=fopen(file.c_str(),"rb");		// Open file for reading
    if (fp){							// if file opened successfully
        fseek(fp,0,SEEK_END);			// Find out filesize
        str.resize(ftell(fp));			// Resize string in advance
        rewind(fp);						// Get back to start of the file
        fread(&str[0],1,str.size(),fp); //Read the whole file in the string
        fclose(fp);						// Close file
    }
	else
	{	
		// We can fail to load the file, which is unlikely actually
		std::wstring err = L"Raytracer engine has failed to load obj!\nUnable to find or open the file: \"";
		err+=std::wstring(file.begin(),file.end()); // Avoid using printf with something that user can mess around with!
		err+=L"\"";
		MessageBox(NULL, err.c_str(), L"Obj Loading Failed", MB_OK|MB_ICONWARNING);
		return;
	} 

	// This is one of most horrendous, unloved pieces of code I produced
	// Beware strange design decisions. I will now commit sudoku.
	// If anything, this should be rewritten first, to suit proper needs, and proper file format
	// We will discard UVmaps(no materials in engine), normals(we calculate our own)
	char sep[] = " /\n\r";	// Separator symbols
	std::string smallstr="";// I know, it's sort of painful to it this way, but here we go...
	int rm = 0;				// Reading mode
	int vc = 0;				// Vertex count(vector of vertices)
	//int oc = 0;				// Object count(scene)
	std::vector<vector*> vv;	// Vectices in a vector
	// 0 - Expecting defining token(v,vn,vp,f)
	// 1,4,7 - reading face, expecting first, 4 for UV, 7 for normal
	// 2,5,8 - -//- second
	// 3,6,9 - -//- third
	// 10 - reading vertex, first number of vector
	// 11 - reading v2
	// 12 - v3
	int		i1,i2,i3; // Numbers corresponding to triangles
	int		u1,u2,u3; // Numbers corresponding to UV coords - DEPREC
	int		n1,n2,n3; // Numbers corresponding to normals   - DEPREC
	float	p1,p2,p3;
	vector	v1,v2,v3;
    for (unsigned int i=0; i< str.length(); i++)//Iterate over string
    {
        if (strchr(sep,(int)str[i])!= NULL || i==str.length()-1) // Did we find separator?
        {
			if(i==str.length()-1 && strchr(sep,(int)str[i])== NULL)smallstr+=str[i]; // Write symbol to smallstr if end
            {
				// WARNING: A LOT OF REDUNDANCY
				switch(rm)
				{
				case 0:
					if(smallstr.compare(std::string("f"))==0)rm=1;// Read face
					if(smallstr.compare(std::string("v"))==0)rm=10;// Read vertex
					break;
				case 1:
					sscanf(smallstr.c_str(),"%i",&i1);

					if (str[i]=='/') rm=4; else rm=2; break;
				case 4:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&u1);
					if (str[i]=='/') rm=7; else rm=2; break;
				case 7:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&n1);
					if (str[i]=='/') rm=0; else rm=2; break;

				case 2:
					sscanf(smallstr.c_str(),"%i",&i2);
					if (str[i]=='/') rm=5; else rm=3; break;
				case 5:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&u2);
					if (str[i]=='/') rm=8; else rm=3; break;
				case 8:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&n2);
					if (str[i]=='/') rm=0; else rm=3; break;

				case 3:
					sscanf(smallstr.c_str(),"%i",&i3);
					if (str[i]=='/') rm=6; 
					else{sc.sceneobjects[oindex] = new Triangle(*vv[i1-1],*vv[i2-1],*vv[i3-1],color,refl,refr,diff,spec);sc.sceneobjects[oindex]->light = false;oindex++;rm=0;};
					break;
				case 6:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&u3);
					if (str[i]=='/') rm=9;
					else{sc.sceneobjects[oindex] = new Triangle(*vv[i1-1],*vv[i2-1],*vv[i3-1],color,refl,refr,diff,spec);sc.sceneobjects[oindex]->light = false;oindex++;rm=0;}; 
					break;
				case 9:
					if (!smallstr.empty()) sscanf(smallstr.c_str(),"%i",&n3);
					if (str[i]=='/') rm=0;
					else{sc.sceneobjects[oindex] = new Triangle(*vv[i1-1],*vv[i2-1],*vv[i3-1],color,refl,refr,diff,spec);sc.sceneobjects[oindex]->light = false;oindex++;rm=0;};
					break;
				case 10:
					sscanf(smallstr.c_str(),"%f",&p1); rm=11;break;
				case 11:
					sscanf(smallstr.c_str(),"%f",&p2); rm=12;break;
				case 12:
					sscanf(smallstr.c_str(),"%f",&p3); 
					vc++; vv.push_back(new vector(p1,p2,p3));
					rm=0; break;
				};

                smallstr=""; // Nullify current word
            }
        }
        else
		{
			smallstr+=str[i]; // Write letter to word
		}
           
    }
};

// Bmp is practically raw data
// No compression, no loss, exceptionally simple to work with
void raytracer::SaveRenderImage(std::string file, CanvasData &canv)
{
	int w			= canv.GetWidth();
	int h			= canv.GetHeight();
	int alignedw	= ((w * 3 + 3) & 0xfffffffc); // (sic!) measured in bytes!

	FILE *fp;
	fp=fopen(file.c_str(),"wb");		// Open file for reading
    if (fp){							// if file opened successfully
        BITMAPINFOHEADER BMIH;
        BMIH.biSize			= sizeof(BITMAPINFOHEADER);
        BMIH.biSizeImage	= h * alignedw; // we must align rows to 4 bytes format
        // Create the bitmap for this OpenGL context
        BMIH.biSize			= sizeof(BITMAPINFOHEADER);
        BMIH.biWidth		= w;
        BMIH.biHeight		= h;
        BMIH.biPlanes		= 1;
        BMIH.biBitCount		= 24;
        BMIH.biCompression = BI_RGB;

        BITMAPFILEHEADER bmfh;
        int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize;
        LONG lImageSize = BMIH.biSizeImage;
        LONG lFileSize = nBitsOffset + lImageSize;
        bmfh.bfType = 'B'+('M' << 8 );	// BMP 'magic number'
        bmfh.bfOffBits = nBitsOffset;
        bmfh.bfSize = lFileSize;
        bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

        //Write the bitmap file header
        UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), fp);
        //And then the bitmap info header
        UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 1, sizeof(BITMAPINFOHEADER), fp);
        //Finally, write the image data itself
		// Dirty tactics!
		std::string BGR_BMP = "";
		BGR_BMP.resize(h*alignedw);

		for(int i = 0; i < h; i++) 
		{
			for(int j = 0; j < w; j++)
			{
				BGR_BMP[i*alignedw+j*3  ] = unsigned char((canv.pixels[(h-1-i)*w+j]&0x000000FF));		// Blue
				BGR_BMP[i*alignedw+j*3+1] = unsigned char((canv.pixels[(h-1-i)*w+j]&0x0000FF00)>>8);	// Green
				BGR_BMP[i*alignedw+j*3+2] = unsigned char((canv.pixels[(h-1-i)*w+j]&0x00FF0000)>>16);	// Red		
			}
			for (int f = 0; f < alignedw-w*3; f++)	// For anything out of picture size, but before end of alignment
				BGR_BMP[i*alignedw+w*3+f]=0;
		}
		fwrite((BGR_BMP.c_str()), h*alignedw, sizeof(unsigned char), fp);
        fclose(fp);
    }
	else
	{	
		// We can fail to load the file, which is unlikely actually
		std::wstring err = L"Raytracer engine has failed to save image!\nUnable to access or open the file for writing: \"";
		err+=std::wstring(file.begin(),file.end()); // Avoid using printf with something that user can mess around with!
		err+=L"\"";
		MessageBox(NULL, err.c_str(), L"File saving failed", MB_OK|MB_ICONWARNING);
		return;
	} 
}