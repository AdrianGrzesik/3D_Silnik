#include "olcConsoleGameEngine.h"
#include <fstream>
#include <strstream>
#include <algorithm>
using namespace std;

//testowy projekt do zabawy .
//deklaracja wektore 3d
struct vec3d
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1; // 4 stała do macierzy
};

// deklaracja trójkata z 3 wektorów
struct triangle
{
	vec3d p[3];

	wchar_t sym;
	short col;
};

struct mesh
{
	vector<triangle> tris;
	//ładowanie obrazu z pliku
	bool LoadFromObjectFile(string sFilename)
	{// sprawdz aistnienie pliku
		ifstream f(sFilename);
		if (!f.is_open())
			return false;


		vector<vec3d> verts;
		//pobieranie znaków
		while (!f.eof()) {
			//linijka po linijce
			char line[128];
			f.getline(line, 128);

			strstream s;
			s << line;
			//wektory
			char junk;
			if (line[0] == 'v') {
				vec3d v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}
			//trójkaty
			if (line[0] == 'f') {

				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
			}

		}


		return true;
	}
};

// deklaracja macierzy 4x4 do obliczen 3d
struct mat4x4
{
	float m[4][4] = { 0 };
};

// deklaracja klasy wykonujacej operacje graficzne
class olcEngine3D : public olcConsoleGameEngine
{
public:
	olcEngine3D()
	{
		m_sAppName = L"3D Demo";

	}

private:
	// szescian
	mesh meshCube;
	//matrix projekcji 3d
	mat4x4 matProj;
	// kat kamery
	vec3d vCamera;
	vec3d vLookDirection;
	float fYaw;
	float fXaw;
	float fZaw;
	float fFov = 90;
	vec3d vUp = { 0,1,0 };
	vec3d vTarget = { 0,0,1 };
	vec3d vRight = { 1,0,0 };

	// zmienna czasowa
	float fTheta;
	//funkcja przekształacjaca wektor przez dowolną macierz 
	vec3d Matrix_MultiplyVector(mat4x4& m, vec3d& i)
	{
		vec3d v;
		v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
		v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
		v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
		v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
		return v;
	}
	// wektor identyczny
	mat4x4 Matrix_MakeIdentity()
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}
	//macierz rotacji po osi x
	mat4x4 Matrix_MakeRotationX(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[1][2] = sinf(fAngleRad);
		matrix.m[2][1] = -sinf(fAngleRad);
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}
	// macierz rotacji po osi y
	mat4x4 Matrix_MakeRotationY(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][2] = sinf(fAngleRad);
		matrix.m[2][0] = -sinf(fAngleRad);
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = cosf(fAngleRad);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}
	//macierz roacji po osi z
	mat4x4 Matrix_MakeRotationZ(float fAngleRad)
	{
		mat4x4 matrix;
		matrix.m[0][0] = cosf(fAngleRad);
		matrix.m[0][1] = sinf(fAngleRad);
		matrix.m[1][0] = -sinf(fAngleRad);
		matrix.m[1][1] = cosf(fAngleRad);
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_MakeTranslation(float x, float y, float z)
	{
		mat4x4 matrix;
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		matrix.m[3][0] = x;
		matrix.m[3][1] = y;
		matrix.m[3][2] = z;
		return matrix;
	}
	//macierz matrixa projekcji
	mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
	{
		float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
		mat4x4 matrix;
		matrix.m[0][0] = fAspectRatio * fFovRad;
		matrix.m[1][1] = fFovRad;
		matrix.m[2][2] = fFar / (fFar - fNear);
		matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matrix.m[2][3] = 1.0f;
		matrix.m[3][3] = 0.0f;
		return matrix;
	}
	//nakładanie się 2 efektów 
	mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
	{
		mat4x4 matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
		return matrix;
	}
	//wskaźnik (obecne położenie, cel, sposób)
	mat4x4 Matrix_PointAt(vec3d& pos, vec3d& target, vec3d& up) {
		//nowy przód suna wektorów
		vec3d newForward = Vector_Sub(target, pos);
		//podobienstwo/dlugosc (jednostkowanie)
		newForward = Vector_Normalise(newForward);

		//nowa góra
		vec3d a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
		vec3d newUp = Vector_Sub(up, a);
		newUp = Vector_Normalise(newUp);

		//nowe prawo
		vec3d b = Vector_CrossProduct(newUp, newForward);
		vec3d newRight = Vector_Normalise(b);

		//matrix translacji
		mat4x4 matrix;
		matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
		return matrix;
	}

	mat4x4 Matrix_QuickInverse(mat4x4& m) // obrót/ translacja matrycy
	{
		mat4x4 matrix;
		matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
		matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
		matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
		matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
		matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
		matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
		matrix.m[3][3] = 1.0f;
		return matrix;
	}
	//dodawanie wektorów
	vec3d Vector_Add(vec3d& v1, vec3d& v2)
	{
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}
	//odejmowanie
	vec3d Vector_Sub(vec3d& v1, vec3d& v2)
	{
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}
	//mnożenie
	vec3d Vector_Mul(vec3d& v1, float k)
	{
		return { v1.x * k, v1.y * k, v1.z * k };
	}
	//dzielenie
	vec3d Vector_Div(vec3d& v1, float k)
	{
		return { v1.x / k, v1.y / k, v1.z / k };
	}
	//podobienstwo wektorów
	float Vector_DotProduct(vec3d& v1, vec3d& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}
	//długość
	float Vector_Length(vec3d& v)
	{
		return sqrtf(Vector_DotProduct(v, v));
	}
	//podobienstwo / długosc = normalizacja
	vec3d Vector_Normalise(vec3d& v)
	{
		float l = Vector_Length(v);
		return { v.x / l, v.y / l, v.z / l };
	}
	//vortex ( prostopadła do płaszczyzny wektorów)
	vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
	{
		vec3d v;
		v.x = v1.y * v2.z - v1.z * v2.y;
		v.y = v1.z * v2.x - v1.x * v2.z;
		v.z = v1.x * v2.y - v1.y * v2.x;
		return v;
	}

	//CLIPPING 
	vec3d Vector_IntersectPlane(vec3d& plane_p, vec3d& plane_n, vec3d& lineStart, vec3d& lineEnd)
	{
		plane_n = Vector_Normalise(plane_n);
		float plane_d = -Vector_DotProduct(plane_n, plane_p);
		float ad = Vector_DotProduct(lineStart, plane_n);
		float bd = Vector_DotProduct(lineEnd, plane_n);
		float t = (-plane_d - ad) / (bd - ad);
		vec3d lineStartToEnd = Vector_Sub(lineEnd, lineStart);
		vec3d lineToIntersect = Vector_Mul(lineStartToEnd, t);
		return Vector_Add(lineStart, lineToIntersect);
	}

	//CLIPPING !!!!!!??????
	int Triangle_ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle& in_tri, triangle& out_tri1, triangle& out_tri2)
	{
		// sprawdzam prostą
		plane_n = Vector_Normalise(plane_n);

		// in or out ?
		auto dist = [&](vec3d& p)
		{
			vec3d n = Vector_Normalise(p);
			return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p));
		};

		//schowek
		vec3d* inside_points[3];  int nInsidePointCount = 0;
		vec3d* outside_points[3]; int nOutsidePointCount = 0;

		//licze dystans
		float d0 = dist(in_tri.p[0]);
		float d1 = dist(in_tri.p[1]);
		float d2 = dist(in_tri.p[2]);

		//dla wszystkich trzech punktów trójkąta
		if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[0]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[0]; }
		if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[1]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[1]; }
		if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[2]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.p[2]; }

		//rozważamy przypadki
		if (nInsidePointCount == 0)
		{
			// nic nie robi
			return 0;
		}

		if (nInsidePointCount == 3)
		{
			// traktuje go jak jeden wadliwy trójkąt
			out_tri1 = in_tri;

			return 1;
		}

		if (nInsidePointCount == 1 && nOutsidePointCount == 2)
		{
			//?????
			out_tri1.col = in_tri.col;
			out_tri1.sym = in_tri.sym;

			out_tri1.p[0] = *inside_points[0];


			out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

			return 1;
		}

		if (nInsidePointCount == 2 && nOutsidePointCount == 1)
		{
			///??????
			out_tri1.col = in_tri.col;
			out_tri1.sym = in_tri.sym;

			out_tri2.col = in_tri.col;
			out_tri2.sym = in_tri.sym;


			out_tri1.p[0] = *inside_points[0];
			out_tri1.p[1] = *inside_points[1];
			out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

			out_tri2.p[0] = *inside_points[1];
			out_tri2.p[1] = out_tri1.p[2];
			out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

			return 2;
		}
	}

	// shadery ( cieniowanie )
	CHAR_INFO GetColour(float lum)
	{
		short bg_col, fg_col;
		wchar_t sym;
		int pixel_bw = (int)(13.0f * lum);
		switch (pixel_bw)
		{
		case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

		case 1: bg_col = BG_BLACK; fg_col = FG_GREEN; sym = PIXEL_QUARTER; break;
		case 2: bg_col = BG_BLACK; fg_col = FG_GREEN; sym = PIXEL_HALF; break;
		case 3: bg_col = BG_BLACK; fg_col = FG_GREEN; sym = PIXEL_THREEQUARTERS; break;
		case 4: bg_col = BG_BLACK; fg_col = FG_GREEN; sym = PIXEL_SOLID; break;

		case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREEN; sym = PIXEL_QUARTER; break;
		case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREEN; sym = PIXEL_HALF; break;
		case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREEN; sym = PIXEL_THREEQUARTERS; break;
		case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREEN; sym = PIXEL_SOLID; break;

		case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
		case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
		case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
		case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
		default:
			bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
		}

		CHAR_INFO c;
		c.Attributes = bg_col | fg_col;
		c.Char.UnicodeChar = sym;
		return c;
	}

public:
	//pierwsza generacja
	bool OnUserCreate() override
	{

		//pobranie z pliku
		meshCube.LoadFromObjectFile("test1.obj");
		//matProj jako wektor z perspektywą
		matProj = Matrix_MakeProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);

		return true;
	}
	// render obrazu w czasie rzeczywistym
	bool OnUserUpdate(float fElapsedTime) override
	{
		//matProj jako wektor z perspektywą
		matProj = Matrix_MakeProjection(fFov, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);
		vec3d vCamRight = Vector_CrossProduct(vLookDirection, vUp);
		 vCamRight = Vector_Mul(vCamRight, 8.0f * fElapsedTime);
		vec3d vCamUp = Vector_CrossProduct(vLookDirection, vCamRight);
		 vCamUp = Vector_Mul(vCamUp, 80.0f * fElapsedTime);

		//bind strzałki w górę
		if (GetKey(VK_UP).bHeld) {
			//vCamera.y += 5.0f * fElapsedTime;
			vCamera = Vector_Add(vCamera, vCamUp);
			/*vCamera.y -= vUp.y * fElapsedTime *5.0f;
			vCamera.x -= vUp.x * fElapsedTime*5.0f;
			vCamera.z -= vUp.z * fElapsedTime* 5.0f;*/
		}

		//bind strzałki w dół
		if (GetKey(VK_DOWN).bHeld) {
			//vCamera.y -= 5.0f * fElapsedTime;
			vCamera = Vector_Sub(vCamera, vCamUp);
			/*vCamera.y += vUp.y * fElapsedTime * 5.0f;
			vCamera.x += vUp.x * fElapsedTime * 5.0f;
			vCamera.z += vUp.z * fElapsedTime * 5.0f;*/
		}

		//bind strzałki w lewo
		if (GetKey(VK_LEFT).bHeld)
		{
			vCamera = Vector_Add(vCamera, vCamRight);
			//vCamera.x -= 5.0f * fElapsedTime;
			/*vCamera.y -= vRight.y * fElapsedTime * 5.0f;
			vCamera.x -= vRight.x * fElapsedTime * 5.0f;
			vCamera.z -= vRight.z * fElapsedTime * 5.0f;*/
		}

		//bind strzałki w prawo
		if (GetKey(VK_RIGHT).bHeld)
		{
			//vCamera.x += 5.0f * fElapsedTime;
			vCamera = Vector_Sub(vCamera, vCamRight);
			//vCamera.y += vRight.y * fElapsedTime * 5.0f;
			//vCamera.x += vRight.x * fElapsedTime * 5.0f;
			//vCamera.z += vRight.z * fElapsedTime * 5.0f;
		}

		vec3d vForward = Vector_Mul(vLookDirection, 8.0f * fElapsedTime);
		//wokół osi y
		if (GetKey(L'A').bHeld)
			fYaw += 2.0f * fElapsedTime;

		if (GetKey(L'D').bHeld)
			fYaw -= 2.0f * fElapsedTime;
		//wokoł osi x
		if (GetKey(L'W').bHeld)
			fXaw += 2.0f * fElapsedTime;

		if (GetKey(L'S').bHeld)
			fXaw -= 2.0f * fElapsedTime;
		//wokół osi z
		if (GetKey(L'Z').bHeld)
			fZaw -= 2.0f * fElapsedTime;


		if (GetKey(L'X').bHeld)
			fZaw += 2.0f * fElapsedTime;

		
			//przód tył
		if (GetKey(L'Q').bHeld)
			vCamera = Vector_Add(vCamera, vForward);

		if (GetKey(L'E').bHeld)
			vCamera = Vector_Sub(vCamera, vForward);

		//focus (zoom)
		if (GetKey(L'C').bHeld)
			fFov += 20.0f * fElapsedTime;

		if (GetKey(L'V').bHeld)
			fFov -= 20.0f * fElapsedTime;

		{	// co tik maże tablice
			Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);
			//deklaracja
			mat4x4 matRotZ, matRotX;
			//fTheta += 1.0f * fElapsedTime; 
			matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f);
			matRotX = Matrix_MakeRotationX(fTheta);

			mat4x4 matWorld;
			matWorld = Matrix_MultiplyMatrix(matRotZ, matRotX); // Transform by rotation
			matWorld = Matrix_MultiplyMatrix(matWorld, matRotZ); // Transform by translation

			// Matrix PointAT
			//deklaracje
			vec3d vUp = { 0,1,0 };
			vec3d vTarget = { 0,0, -1 };
			vec3d vRight = { 1,0,0 };
			mat4x4 matCameraRot;
			//obroty
			mat4x4 matCamRotY = Matrix_MakeRotationY(fYaw);
			mat4x4 matCamRotX = Matrix_MakeRotationX(fXaw);
			mat4x4 matCamRotZ = Matrix_MakeRotationZ(fZaw);
			//manipulije macierz
			matCameraRot = matCamRotZ;
			matCameraRot = Matrix_MultiplyMatrix(matCameraRot, matCamRotY );
			matCameraRot = Matrix_MultiplyMatrix(matCameraRot, matCamRotX);
			// wektor przez nią
			vLookDirection = Matrix_MultiplyVector(matCameraRot, vTarget);
			// ustawiam nowy target
			vTarget = Vector_Add(vCamera, vLookDirection);
			/*vUp = Vector_CrossProduct(vTarget, vRight);
			vUp = Vector_Normalise(vUp);*/
			vUp = Matrix_MultiplyVector(matCamRotZ, vUp);
			//vRight = Vector_CrossProduct(vTarget, vUp);
			//vRight = Vector_Normalise(vRight);
			mat4x4 matCamera = Matrix_PointAt(vCamera, vTarget, vUp);

			// matrix kamery
			mat4x4 matView = Matrix_QuickInverse(matCamera);

			// deklaracja stosu kolejki do utworzenia
			vector<triangle> vecTrianglesToRaster;

			// rysowanie trójkatów, 
			for (auto tri : meshCube.tris)
			{	//deklaracje
				triangle triProjected, triTransformed, triViewed;

				// World Matrix Transform
				triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
				triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
				triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

				// deklaracje
				vec3d normal, line1, line2;

				// Get lines either side of triangle
				line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
				line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

				// prostopadła do płąszczyzny trójkąta
				normal = Vector_CrossProduct(line1, line2);

				// jedykowane
				normal = Vector_Normalise(normal);

				//widok z kamery
				vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);

				// cienowanie ( srawdzanie czy światło pada na ścianke
				if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
				{

					// Kierunek promieni świetlnych
					vec3d light_direction = { -1.0f, 0.0f, 1.0f };
					light_direction = Vector_Normalise(light_direction);

					//porównanie nachylenia ściany do promieni światla
					float dp = max(0.1f, Vector_DotProduct(light_direction, normal));

					//cieniowanie
					CHAR_INFO c = GetColour(dp);
					triTransformed.col = c.Attributes;
					triTransformed.sym = c.Char.UnicodeChar;

					//world--> view
					triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
					triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
					triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);
					triViewed.sym = triTransformed.sym;
					triViewed.col = triTransformed.col;

					// do CLIPPINGU
					int nClippedTriangles = 0;
					triangle clipped[2];
					nClippedTriangles = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);

					for (int n = 0; n < nClippedTriangles; n++)
					{
						//  3D --> 2D
						triProjected.p[0] = Matrix_MultiplyVector(matProj, clipped[n].p[0]);
						triProjected.p[1] = Matrix_MultiplyVector(matProj, clipped[n].p[1]);
						triProjected.p[2] = Matrix_MultiplyVector(matProj, clipped[n].p[2]);
						triProjected.col = clipped[n].col;
						triProjected.sym = clipped[n].sym;

						
						triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
						triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
						triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);


						// Offset verts into visible normalised space
						vec3d vOffsetView = { 1,1,0 };
						triProjected.p[0] = Vector_Add(triProjected.p[0], vOffsetView);
						triProjected.p[1] = Vector_Add(triProjected.p[1], vOffsetView);
						triProjected.p[2] = Vector_Add(triProjected.p[2], vOffsetView);
						triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
						triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
						triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
						triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
						triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
						triProjected.p[2].y *= 0.5f * (float)ScreenHeight();


						// przechowanie i sortowanie trójkatów
						vecTrianglesToRaster.push_back(triProjected);

					}
				}
			}

			// sortowanie trójkątów od tyłu do frontu ( metoda malarska)
			sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
				{
					float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
					float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
					return z1 > z2;
				});

			//wypełnianie ( zgodnie z metodą malarską ) 
			for (auto& triProjected : vecTrianglesToRaster)
			{
				// Rasterize triangle
				FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
					triProjected.p[1].x, triProjected.p[1].y,
					triProjected.p[2].x, triProjected.p[2].y,
					triProjected.sym, triProjected.col);


			}



			return true;
		}
	}
};

int main()
{
	olcEngine3D demo;
	if (demo.ConstructConsole(200, 180, 4, 4))
		demo.Start();
	return 0;
}

