//main.cpp

// I want to use this matrix to simulate a 3dimensional environment

/*
size_t index = z * height * width + y * width + x;

// Reverse:
x = index % width;
y = (index / width) % height;
z = index / (width * height);
*/

#include <windows.h>
#include <windowsx.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#ifndef UNICODE
#define UNICODE
#error flag -DUNICODE
#endif

ID2D1Factory* Factory               = nullptr;
ID2D1HwndRenderTarget* RenderTarget = nullptr;
ID2D1BitmapRenderTarget* BitmapRT   = nullptr;
ID2D1Bitmap* Bitmap                 = nullptr;

#include "debug.h"

#define TIME_DURATION 500
#define WIDTH         800
#define HEIGHT        600

std::atomic<bool> olcConsoleGameEngine::m_bAtomActive{false};

// debugging idea...
struct MatrixRain : public olcConsoleGameEngine
{
    virtual void OnUserUpdate()
    {
        Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		Draw(0, 0, L'0', FG_RED);

    }
};

#define GetInstance() GetModuleHandle(NULL)
#define CHECK_MATRIX_POPULATION() do{if(!matrix){return DefWindowProc(hwnd, msg, wp, lp);}}while(0)
#define CHECK_MATRIX_WALLS() x == 0 || x == w -1 || y == 0 || y == h -1 || z == 0 || z == d -1

struct COORD3D{ uint32_t x; uint32_t y; uint32_t z; COORD3D(uint32_t _x, uint32_t _y, uint32_t _z) : x(_x), y(_y), z(_z) {} };

struct Vec3D
{
	float x; float y; float z; Vec3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	Vec3D operator-(const Vec3D& other) const {return {x - other.x, y - other.y, z - other.z};}
    Vec3D operator*(float scalar      ) const {return {x * scalar, y * scalar, z * scalar};   }
    Vec3D operator+(const Vec3D& other) const {return {x + other.x, y + other.y, z + other.z};}
	Vec3D operator/(const Vec3D& other) const {return {x / other.x, y / other.y, z / other.z};}
};

struct entity
{
	entity() = default;

	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t z = 0;
};

struct MATRIX
{
public:
	enum element{air, water, wood, fire, metal, Custom, size};

	struct MaterialAttributes
	{
		float density;
		float friction;
		float thermalConductivity; // rate of heat transfer
		float specificHeat; // energy to raise 1 kg by 1 degree Celcius
		float restitution; // bounciness
		float hardness;
		bool  liquid;

		const char* name = nullptr;
	};
	struct cell
	{
		cell() = default;
		cell(element _MaterialType) : MaterialType(_MaterialType) {}
		cell(float _tempurature)    : temperature(_tempurature)   {}
		cell(Vec3D _velocity)       : velocity(_velocity)         {}
		cell(double _pressure)      : pressure(_pressure)         {}

		float temperature = 0.0f;
		Vec3D velocity    = Vec3D(0.0f, 0.0f, 0.0f);
		double pressure   = 0.0;
		bool active = false;

		element MaterialType;
	};
public:
	uint32_t InitMatrix(uint32_t width, uint32_t height, uint32_t depth)
	{
		//void* mamory = malloc(sizeof(cell) * (width * height* depth));
		//cell* CELL_FRONT_BUFFER = new(memory) cell[width * height* depth];

		CELL_FRONT_BUFFER = std::unique_ptr<cell[]>(new cell[width * height * depth]);
		CELL_BACK_BUFFER  = std::unique_ptr<cell[]>(new cell[width * height * depth]);
		matAtt     		  = std::unique_ptr<MaterialAttributes[]>(new MaterialAttributes[static_cast<uint32_t>(element::size)]);
		w = width; h = height; d = depth;

		printf("-> cells set to: element::air\n");
		for(uint32_t i=0; i<= width * height * depth -1; ++i)
		{
			//CELL_BACK_BUFFER[i].MaterialType = element::air;
			uint32_t x = i % w; uint32_t y = (i / w) % h; uint32_t z = i / (w * h);
			cell back_buffer_cell = CHECK_MATRIX_WALLS() ? WriteDataTo(x, y, z, cell(element::Custom)) : WriteDataTo(x, y, z, cell(element::air));
		}

		// create an initializer field for attributes of materials to tell the cells what their attributes are
		printf("-> elements::name... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].name 			   = "air";
		matAtt[static_cast<uint32_t>(element::water) ].name 			   = "water";
		matAtt[static_cast<uint32_t>(element::wood)  ].name 			   = "wood";
		matAtt[static_cast<uint32_t>(element::fire)  ].name 			   = "fire";
		matAtt[static_cast<uint32_t>(element::metal) ].name 			   = "metal";
		matAtt[static_cast<uint32_t>(element::Custom)].name 			   = "Custom";

		printf("-> elements::density... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].density 			   = 1.225f;
		matAtt[static_cast<uint32_t>(element::water) ].density 			   = 1000.0f;
		matAtt[static_cast<uint32_t>(element::wood)  ].density 			   = 700.0f;
		matAtt[static_cast<uint32_t>(element::fire)  ].density 			   = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].density 			   = 7800.0f;
		matAtt[static_cast<uint32_t>(element::Custom)].density 			   = 0.0f;
		
		printf("-> elements::friction... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].friction 		   = 0.02f;
		matAtt[static_cast<uint32_t>(element::water) ].friction 		   = 0.1f;
		matAtt[static_cast<uint32_t>(element::wood)  ].friction 		   = 0.4f;
		matAtt[static_cast<uint32_t>(element::fire)  ].friction 		   = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].friction 		   = 0.6f;
		matAtt[static_cast<uint32_t>(element::Custom)].friction 		   = 0.0f;

		printf("-> elements::ThermalConductivity... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].thermalConductivity = 0.025f;
		matAtt[static_cast<uint32_t>(element::water) ].thermalConductivity = 0.6f;
		matAtt[static_cast<uint32_t>(element::wood)  ].thermalConductivity = 0.12f;
		matAtt[static_cast<uint32_t>(element::fire)  ].thermalConductivity = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].thermalConductivity = 400.0f;
		matAtt[static_cast<uint32_t>(element::Custom)].thermalConductivity = 0.0f;

		printf("-> elements::specificHeat... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].specificHeat 	   = 1005.0f;
		matAtt[static_cast<uint32_t>(element::water) ].specificHeat 	   = 4186.0f;
		matAtt[static_cast<uint32_t>(element::wood)  ].specificHeat 	   = 2400.0f;
		matAtt[static_cast<uint32_t>(element::fire)  ].specificHeat 	   = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].specificHeat 	   = 450.0f;
		matAtt[static_cast<uint32_t>(element::Custom)].specificHeat 	   = 0.0f;

		printf("-> elements::restitution... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].restitution 		   = 0.0f;
		matAtt[static_cast<uint32_t>(element::water) ].restitution 		   = 0.1f;
		matAtt[static_cast<uint32_t>(element::wood)  ].restitution 		   = 0.3f;
		matAtt[static_cast<uint32_t>(element::fire)  ].restitution 		   = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].restitution 		   = 0.9f;
		matAtt[static_cast<uint32_t>(element::Custom)].restitution 		   = 0.0f;

		printf("-> elements::hardness... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].hardness 		   = 0.0f;
		matAtt[static_cast<uint32_t>(element::water) ].hardness 		   = 0.1f;
		matAtt[static_cast<uint32_t>(element::wood)  ].hardness 		   = 0.3f;
		matAtt[static_cast<uint32_t>(element::fire)  ].hardness 		   = 0.0f;
		matAtt[static_cast<uint32_t>(element::metal) ].hardness 		   = 0.9f;
		matAtt[static_cast<uint32_t>(element::Custom)].hardness 		   = 10000.0f;
			   
		printf("-> elements::hardness... \n");
		matAtt[static_cast<uint32_t>(element::air)   ].liquid 			   = false;
		matAtt[static_cast<uint32_t>(element::water) ].liquid 			   = true;
		matAtt[static_cast<uint32_t>(element::wood)  ].liquid 			   = false;
		matAtt[static_cast<uint32_t>(element::fire)  ].liquid 			   = false;
		matAtt[static_cast<uint32_t>(element::metal) ].liquid 			   = false;
		matAtt[static_cast<uint32_t>(element::Custom)].liquid 			   = false;

		

		printf("matrix initialized: CELLS: [%d] BPC: [%d]\n\n", (w*h*d), sizeof(CELL_BACK_BUFFER[0]));
		return width * height * depth;
	}

	cell AccessDataAt(uint32_t x, uint32_t y, uint32_t z)
	{ 
		x = x>=w ? w-1 : x; y = y>=h ? h-1 : y; z = z>=d ? d-1 : z;
		return CELL_BACK_BUFFER[FlattenedIndex(x, y, z)]; // [x][y][z]
	}

	cell WriteDataTo(uint32_t x, uint32_t y, uint32_t z, const cell& _data)
	{
		x = x>=w ? w-1 : x; y = y>=h ? h-1 : y; z = z>=d ? d-1 : z;
		CELL_BACK_BUFFER[FlattenedIndex(x, y, z)] = _data; // [x][y][z]
		return CELL_BACK_BUFFER[FlattenedIndex(x, y, z)];
	}

	inline void UpdateWorldView(HWND hwnd, std::atomic<bool>& running)
	{
		if(running)
		{
			for(uint32_t i=0; i<=w*h*d -1; ++i)
			{
				if(!running){break;}
				
				if(CELL_BACK_BUFFER[i].MaterialType != CELL_FRONT_BUFFER[i].MaterialType)
				{
					// copy the back buffer to the front buffer up until the point at which a cell tranformation was detected
					memcpy(CELL_FRONT_BUFFER.get() + i, CELL_BACK_BUFFER.get() + i, sizeof(cell) * (w*h*d - i));
					InvalidateRect(hwnd, nullptr, FALSE);
					break;
				}
			}
		}
	}

	inline void FlushMatrix(std::atomic<bool>& running, uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2, uint32_t z1, uint32_t z2)
	{
		for(uint32_t z=z1; z<=z2; ++z)
		{
			if(!running) break;

			for(uint32_t y=y1; y<=y2; ++y)
			{
				for(uint32_t x=x1; x<=x2; ++x)
				{
					cell genericCellRead = CHECK_MATRIX_WALLS() ? WriteDataTo(x, y, z, cell(element::Custom)) : cell(element::air);
					MaterialAttributes MA = ReadCellAttributes(genericCellRead); // read that these cells can't be destroyed... pretty much
					//WriteDataTo(x, y, z, cell(static_cast<element>(rand() % element::size)));

					//WriteDataTo(x, AccessDataAt(x, y, z).MaterialType != element::air && AccessDataAt(x, y, z).MaterialType != element::Custom ? y - 0.01 : y, z, cell());
				}
			}
		}
	}

	// THREAD CHUNK FUNCTION //

	inline void UpdateSimulationState(HWND hwnd, std::atomic<bool>* pressed, std::atomic<bool>* held, std::atomic<bool>* released, std::atomic<bool>& running, std::atomic<bool>& ready, uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2, uint32_t z1, uint32_t z2)
	{
		ready = false;

		while(running)
		{
			//iterate across this threads chunk

			FlushMatrix(running, x1, x2, y1, y2, z1, z2);
			// first, flush the entire matrix to a default state every frame, then update it into it's proper state. 
			
			

			for(uint32_t z=z1; z<=z2; ++z)
			{
				if(!running) break;

				for(uint32_t y=y1; y<=y2; ++y)
				{
					for(uint32_t x=x1; x<=x2; ++x)
					{
						
					}
				}
			}

			if(held[VK_SPACE])
			{
				printf("Thread[%d] - Updated [(%d, %d, %d),(%d, %d, %d)]\n", std::this_thread::get_id(), x1, y1, z1, x2, y2, z2);
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(TIME_DURATION));

		}

		ready = true;
	}

	element ExGetElement(cell c){ return c.MaterialType;}

	uint32_t FlattenedIndex(uint32_t x, uint32_t y, uint32_t z)
	{
		x = x>=w ? w-1 : x; y = y>=h ? h-1 : y; z = z>=d ? d-1 : z;
		return z * (w * h) + y * w + x; 
	}
	
	
	std::unique_ptr<cell[]> CELL_FRONT_BUFFER; //allowed to use for visual representation, but let's wrap it in a function later

private:///////////////////////////////////////////////////////////////////////////////////////|
	std::unique_ptr<cell[]> CELL_BACK_BUFFER;    ///////////////////////////////////////////// |
	std::unique_ptr<MaterialAttributes[]> matAtt;///////////////////////////////////////////// |
public://///////////////////////////////////////////////////////////////////////////////////// |

	uint32_t w = 0;
	uint32_t h = 0;
	uint32_t d = 0;

	uint16_t _pixelWidth  = 5;
	uint16_t _pixelHeight = 5;

	uint16_t _zLevel = 0;

	MaterialAttributes ReadCellAttributes(cell c)
	{
		if(c.MaterialType >= element::size){ printf("\nWARNING: MaterialType: element::%d is greater than element::size. Defaulting to element::air", static_cast<uint32_t>(c.MaterialType)); }
		return c.MaterialType < element::size ? matAtt[static_cast<uint32_t>(c.MaterialType)] : matAtt[static_cast<uint32_t>(element::air)];
	}
};

struct Camera
{
	Camera() : FOV(90.0f) {}
public:
	bool TraceRayThroughMatrix(Vec3D rayOrigin, Vec3D rayDir, float maxDistance, bool (*IsCellBlocking)(int, int, int))
	{
		rayDir = normalize(rayDir);

		int x = static_cast<int>(floor(rayOrigin.x));
		int y = static_cast<int>(floor(rayOrigin.y));
		int z = static_cast<int>(floor(rayOrigin.z));

		int stepX = (rayDir.x >= 0) ? 1 : -1;
		int stepY = (rayDir.y >= 0) ? 1 : -1;
		int stepZ = (rayDir.z >= 0) ? 1 : -1;

		float tDeltaX = (rayDir.x != 0) ? fabs(1.0f / rayDir.x) : INFINITY;
		float tDeltaY = (rayDir.y != 0) ? fabs(1.0f / rayDir.y) : INFINITY;
		float tDeltaZ = (rayDir.z != 0) ? fabs(1.0f / rayDir.z) : INFINITY;

		float voxelBorderX = (stepX > 0) ? (floor(rayOrigin.x) + 1.0f) : floor(rayOrigin.x);
		float voxelBorderY = (stepY > 0) ? (floor(rayOrigin.y) + 1.0f) : floor(rayOrigin.y);
		float voxelBorderZ = (stepZ > 0) ? (floor(rayOrigin.z) + 1.0f) : floor(rayOrigin.z);

		float tMaxX = (rayDir.x != 0) ? fabs((voxelBorderX - rayOrigin.x) / rayDir.x) : INFINITY;
		float tMaxY = (rayDir.y != 0) ? fabs((voxelBorderY - rayOrigin.y) / rayDir.y) : INFINITY;
		float tMaxZ = (rayDir.z != 0) ? fabs((voxelBorderZ - rayOrigin.z) / rayDir.z) : INFINITY;

		float traveled = 0.0f;

		while (traveled < maxDistance)
		{
			if (IsCellBlocking(x, y, z)) { return true; }

			if (tMaxX < tMaxY && tMaxX < tMaxZ)
			{
				x += stepX;
				traveled = tMaxX;
				tMaxX += tDeltaX;
			}
			else if (tMaxY < tMaxZ)
			{
				y += stepY;
				traveled = tMaxY;
				tMaxY += tDeltaY;
			}
			else 
			{
				z += stepZ;
				traveled = tMaxZ;
				tMaxZ += tDeltaZ;
			}
		}

		return false; // didn't hit anything
	}

	inline void SetViewPosition(float _fov, Vec3D _position = {0.0f, 0.0f, 0.0f}, Vec3D _lookAt = {0.0f, 0.0f, 1.0f})
	{
		position = _position; lookAt = _lookAt; FOV = _fov;

		forward = normalize(lookAt - position);
		worldUp = {0, 1, 0};
		right   = normalize(Cross(forward, worldUp));
		up      = Cross(right, forward);
	}
	inline void CastRay(uint32_t nScreenWidth, uint32_t nScreenHeight, bool (*IsCellBlocking)(int, int, int))
	{
		float aspectRatio = static_cast<float>(nScreenWidth) / static_cast<float>(nScreenHeight);
		float fovRadians   = FOV * (3.14159f / 180.0f);
		
		float screenPlaneHalfWidth  = tanf(fovRadians / 2.0f);
		float screenPlaneHalfHeight = screenPlaneHalfWidth / aspectRatio;

		for(int j=0; j<=nScreenHeight -1; ++j)
		{
			for(int i=0; i<=nScreenWidth -1; ++i)
			{
				float x 	  = (i + 0.5f) / nScreenWidth;
				float y 	  = (j + 0.5f) / nScreenHeight;

				float screenX = (2.0f * x - 1.0f) * screenPlaneHalfWidth;
				float screenY = (1.0f - 2.0f * y) * screenPlaneHalfHeight;

				Vec3D rayDir  = forward + right * screenX + up * screenY;
				rayDir        = normalize(rayDir);

				while(0)
				{
					bool bHit = TraceRayThroughMatrix(position, rayDir, 100.0f, IsCellBlocking);

					if(bHit)
					{
						break;
					}
				}
			}
		}
	}
private:
	Vec3D normalize(Vec3D vec)
	{
		float len = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
		return {vec.x / len, vec.y / len, vec.z / len};
	}
	Vec3D Cross(Vec3D a, Vec3D b)
	{
	    return
		{
			a.y * b.z - a.z * b.y, 
			a.z * b.x - a.x * b.z, 
			a.x * b.y - a.y * b.x
		};
	}
protected:
	Vec3D forward  = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D worldUp  = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D right    = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D up       = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D position = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D lookAt   = Vec3D(0.0f, 0.0f, 0.0f);
	float FOV;
};

namespace WINDOWGraphicsOverlay
{
	COLORREF GetMaterialColor(MATRIX::element e)
	{
		switch(e)
		{
			case MATRIX::element::air:   return RGB(173, 216, 230); break;
			case MATRIX::element::water: return RGB(0, 0, 255);     break;
			case MATRIX::element::wood:  return RGB(139, 69, 19);   break;
			case MATRIX::element::fire:  return RGB(255, 0, 0);     break;
			case MATRIX::element::metal: return RGB(169, 169, 169); break;
			default:                	 return RGB(0, 0, 0); break;
		}
	}

	bool blitOverlay(HWND hwnd, uint32_t zLevel, int32_t cellWidth, int32_t cellHeight)
	{
		printf("BitmapRT = %p\n", BitmapRT);
		MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		if(!BitmapRT)
		{
			printf("bad BitmapRT");
			return false;
		}

		BitmapRT->BeginDraw();
		BitmapRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		for (int j = 0; j < matrix->h; ++j)
    	{
    		for (int i = 0; i < matrix->w; ++i)
    		{
				COLORREF CellColor = WINDOWGraphicsOverlay::GetMaterialColor(matrix->CELL_FRONT_BUFFER[matrix->FlattenedIndex(i, j, zLevel)].MaterialType);
				D2D1_COLOR_F color = {GetRValue(CellColor) / 255.0f, GetGValue(CellColor) / 255.0f, GetBValue(CellColor) / 255.0f, 1.0f};
				ID2D1SolidColorBrush* pBrush = nullptr;
				BitmapRT->CreateSolidColorBrush(color, &pBrush);
				D2D1_RECT_F rect = D2D1::RectF(
    				static_cast<float>(i * cellWidth),
    				static_cast<float>(j * cellHeight),
    				static_cast<float>((i + 1) * cellWidth),
    				static_cast<float>((j + 1) * cellHeight)
    			);
    			BitmapRT->FillRectangle(&rect, pBrush);
    			pBrush->Release();
			}
		}
		BitmapRT->EndDraw();
		Bitmap->Release();
		HRESULT hr = BitmapRT->GetBitmap(&Bitmap);
		if(FAILED(hr))
		{
			wchar_t errorMsg[512];
       		swprintf_s(errorMsg, L"D2D1CreateFactory failed with HRESULT: 0x%08X", hr);
       		MessageBoxW(0, errorMsg, L"Error", MB_ICONERROR);
		}
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		if (msg == WM_NCCREATE)
    	{
    	    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lp);
    	    MATRIX* matrix = static_cast<MATRIX*>(pcs->lpCreateParams);
    	   	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(matrix));

			return 1;
    	}

		if(msg == WM_NCDESTROY)
		{
			MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			delete matrix;
			matrix=nullptr;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);

			if(RenderTarget){RenderTarget->Release();}
			if(Factory)     {Factory->Release();     }
		}

		switch(msg)
		{
			case WM_CREATE:
			{
				HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory);
				if(FAILED(hr))
				{
					wchar_t errorMsg[512];
        			swprintf_s(errorMsg, L"D2D1CreateFactory failed with HRESULT: 0x%08X", hr);
        			MessageBoxW(0, errorMsg, L"Error", MB_ICONERROR);
				}

				FLOAT dpiX = 96.0f;
				FLOAT dpiY = 96.0f;
				//Factory->GetDesktopDpi(&dpiX, &dpiY);

				RECT rc;
				GetClientRect(hwnd, &rc);

				D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
    				D2D1_RENDER_TARGET_TYPE_HARDWARE,
    				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
    				dpiX, dpiY
				);

				D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = {};
				hwndProps.hwnd 			   = hwnd;
				hwndProps.pixelSize		   = size;
				hwndProps.presentOptions   = D2D1_PRESENT_OPTIONS_NONE;

				if(Factory)
				{
					hr = Factory->CreateHwndRenderTarget(props, hwndProps, &RenderTarget);
				}

				if(FAILED(hr))
				{
					wchar_t errorMsg[512];
        			swprintf_s(errorMsg, L"D2D1CreateFactory failed with HRESULT: 0x%08X", hr);
        			MessageBoxW(0, errorMsg, L"Error", MB_ICONERROR);
					return FALSE;
				}

				if(RenderTarget)
				{
					RenderTarget->CreateCompatibleRenderTarget(&BitmapRT);
				}
				else{
					printf("RenderTarget->CreateCompatibleRenderTarget(&BitmapRT); RENDERTARGET = nullptr");
				}
			}
			break;
			case WM_SIZE:
			{
			    if (RenderTarget)
			    {
			        UINT width  = LOWORD(lp);
			        UINT height = HIWORD(lp);
			        RenderTarget->Resize(D2D1::SizeU(width, height));
			    }
				else{
					printf("WM_SIZE: RENDERTARGET = nullptr");
				}
			}
			break;
			case WM_MOUSEMOVE:
			{
				MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				CHECK_MATRIX_POPULATION();
				
				uint32_t x = GET_X_LPARAM(lp);
				uint32_t y = GET_Y_LPARAM(lp);

				uint32_t xCell = x / matrix->_pixelWidth;
				uint32_t yCell = y / matrix->_pixelHeight;

				MATRIX::cell readcell = MATRIX::cell();

				if(wp & MK_LBUTTON)
				{
					readcell = matrix->WriteDataTo(xCell, yCell, matrix->_zLevel, MATRIX::cell(MATRIX::element::fire));
					printf("cell[%d, %d][SLICE: %d]->element: %s\n", xCell, yCell, matrix->_zLevel, matrix->ReadCellAttributes(readcell).name);
				}

				if(wp & MK_RBUTTON)
				{
					readcell = matrix->AccessDataAt(xCell, yCell, matrix->_zLevel);
					printf("cell[%d, %d][SLICE: %d]->element: %s\n", xCell, yCell, matrix->_zLevel, matrix->ReadCellAttributes(readcell).name);
				}
			}
			break;
			case WM_KEYDOWN:
			{
				MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				CHECK_MATRIX_POPULATION();
				
				matrix->_zLevel += wp == VK_UP   ? 1 : 0;
				matrix->_zLevel -= wp == VK_DOWN ? 1 : 0;

				InvalidateRect(hwnd, nullptr, FALSE);
			}
			break;
			case WM_PAINT:
			{
				MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				CHECK_MATRIX_POPULATION();

				ValidateRect(hwnd, nullptr);

				Camera cam;

				//RenderTarget->BeginDraw();
				//RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
				//for (int j = 0; j < matrix->h; ++j)
    			//{
    			//	for (int i = 0; i < matrix->w; ++i)
    			//	{
				//		COLORREF CellColor = WINDOWGraphicsOverlay::GetMaterialColor(matrix->CELL_FRONT_BUFFER[matrix->FlattenedIndex(i, j, matrix->_zLevel)].MaterialType);
				//		D2D1_COLOR_F color = {GetRValue(CellColor) / 255.0f, GetGValue(CellColor) / 255.0f, GetBValue(CellColor) / 255.0f, 1.0f};
				//		ID2D1SolidColorBrush* pBrush = nullptr;
				//		RenderTarget->CreateSolidColorBrush(color, &pBrush);
				//		D2D1_RECT_F rect = D2D1::RectF(
    			//			static_cast<float>(i * matrix->_pixelWidth),
    			//			static_cast<float>(j * matrix->_pixelHeight),
    			//			static_cast<float>((i + 1) * matrix->_pixelWidth),
    			//			static_cast<float>((j + 1) * matrix->_pixelHeight)
    			//		);
    			//		RenderTarget->FillRectangle(&rect, pBrush);
    			//		pBrush->Release();
				//	}
				//}
				//RenderTarget->EndDraw();

				if(RenderTarget)
				{
					RenderTarget->BeginDraw();
					RenderTarget->DrawBitmap(Bitmap, D2D1::RectF(0, 0, static_cast<float>(WIDTH), static_cast<float>(HEIGHT)));
					RenderTarget->EndDraw();
				}
				else{
					printf("WM_PAINT: RENDERTARGET = nullptr");
				}
			}
			break;
			case WM_DESTROY: PostQuitMessage(0); break;
			default: return DefWindowProc(hwnd, msg, wp, lp); break;
		}

		return 0;
	}

	void ShowErrorMessage(const char* errorText) {
		int len = MultiByteToWideChar(CP_ACP, 0, errorText, -1, NULL, 0);
		wchar_t* wideErrorText = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, errorText, -1, wideErrorText, len);
		MessageBoxW(NULL, wideErrorText, L"Error", MB_ICONERROR | MB_OK);
		delete[] wideErrorText;
	}

	void* CreateWindowOverlay(MATRIX* matrix, uint32_t width, uint32_t height)
	{
		void* _handle = nullptr;

		WNDCLASSEX wc = { };
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.lpfnWndProc   = WindowProc;
		wc.hInstance     = GetInstance();
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = L"3DSimulationOverlay";
		if(!RegisterClassEx(&wc))
		{
			printf("-> Window Class not initialized or registered\n");
		}

		_handle = CreateWindowEx(
			0, wc.lpszClassName, 
			L"REDJADER - SIMULATION", 
			WS_OVERLAPPEDWINDOW, 
			CW_USEDEFAULT, CW_USEDEFAULT, 
			/*width * matrix->_pixelWidth + width*0.10*/WIDTH, 
			/*height * matrix->_pixelHeight + height*0.40*/HEIGHT, 
			nullptr, nullptr, GetInstance(), matrix);

		if(_handle==nullptr)
		{
			DWORD errorCode = GetLastError();
    		LPSTR errorText = NULL;
    		FormatMessage(
    		    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    		    NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    		    (LPTSTR)&errorText, 0, NULL);
			
    		printf("Error creating window: %s\n", errorText);
    		LocalFree(errorText);
			return nullptr;
		}
		else
		{
			printf("-> Window Created\n");
		}

		::ShowWindow((HWND)_handle, SW_SHOW);
		::UpdateWindow((HWND)_handle);

		return _handle;
	}
};

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	MATRIX* matrix = new MATRIX();

	MatrixRain MR;

	uint32_t width  = 200;
	uint32_t height = 200;
	uint32_t depth  = 200;

	std::atomic<bool> keyPressed[0xFF]   = { 0 };
	std::atomic<bool> keyReleased[0xFF]  = { 0 };
	std::atomic<bool> keyHeld[0xFF]      = { 0 };
	std::atomic<bool> keyPrev[0xFF]      = { 0 };

	uint32_t SizeOfMatrix = matrix->InitMatrix(width, height, depth);
	HWND _handle = (HWND)WINDOWGraphicsOverlay::CreateWindowOverlay(matrix, width, height);

	std::atomic<bool> running = true;
	std::atomic<bool> ready   = false;
	std::thread::id MainThreadID = std::this_thread::get_id();

	MSG msg = {NULL};

	if(_handle)
	{
		//thread allocation, using smart pointers for now...
		std::unique_ptr<std::thread[]> ChunkThreads = std::unique_ptr<std::thread[]>(new std::thread[std::thread::hardware_concurrency()]);
		
		uint32_t splitPerAxis = std::round(std::cbrt(std::thread::hardware_concurrency()));

		uint32_t chunkSizeX = width  / splitPerAxis;
		uint32_t chunkSizeY = height / splitPerAxis;
		uint32_t chunkSizeZ = depth  / splitPerAxis;

		uint32_t threadIndex = 0;
		for (uint32_t x = 0; x < splitPerAxis; ++x) 
		{
		    for (uint32_t y = 0; y < splitPerAxis; ++y) 
			{
		        for (uint32_t z = 0; z < splitPerAxis; ++z) 
				{
		            uint32_t x1 = x * chunkSizeX;
		            uint32_t x2 = (x == splitPerAxis - 1) ? width : (x + 1) * chunkSizeX;
				
		            uint32_t y1 = y * chunkSizeY;
		            uint32_t y2 = (y == splitPerAxis - 1) ? height : (y + 1) * chunkSizeY;
				
		            uint32_t z1 = z * chunkSizeZ;
		            uint32_t z2 = (z == splitPerAxis - 1) ? depth : (z + 1) * chunkSizeZ;

		            ChunkThreads[threadIndex++] = std::thread
					(
						[=, &keyPressed, &keyHeld, &keyReleased, &running, &ready, x1,  x2,  y1,  y2,  z1, z2](){ matrix->UpdateSimulationState(_handle, keyHeld, keyHeld, keyReleased, running, ready, x1, x2, y1, y2, z1, z2); }
					);
		        }
		    }
		}

		for(uint16_t i=0; i<=std::thread::hardware_concurrency() -1; ++i){
			printf("UpdateThread[%d] -> ", ChunkThreads[i].get_id());
			uint32_t n = ChunkThreads[i].get_id() != MainThreadID ? printf("NewThread\n") : printf("MainThread");
		}

		//MR.ConstructConsole(228, 180, 12, 12);
		//MR.Start(running);

		printf("Starting...\n\n");

		while(running)
		{
			WINDOWGraphicsOverlay::blitOverlay(_handle, matrix->_zLevel, matrix->_pixelWidth, matrix->_pixelHeight);

			while(PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
			{
				
				for (int i = 0; i <= 0xFF - 1; i++)
				{
					short KeyState = GetAsyncKeyState( i );
					bool down      = (KeyState >> 15) & 0x1;

					keyPressed[i]  = !keyPrev[i] && down;
					keyHeld[i]     = keyPrev[i]  && down;
					keyReleased[i] = keyPrev[i]  && !down;

					keyPrev[i] = down;
				}

				if(keyReleased[VK_INSERT])
				{
					running = false;
					printf("Goodbye!\n\n");
					break;
				}

				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			matrix->UpdateWorldView(_handle, running);

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		running = false;

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		for(uint16_t i=0; i<=std::thread::hardware_concurrency() -1; ++i)
		{
			if(ChunkThreads[i].joinable())
			{
				printf("Thread[%d] -> closing\n", ChunkThreads[i].get_id());
				ChunkThreads[i].join();
			}
		}
	}
	else
	{
		MessageBoxW(nullptr, L"WINDOW HANDLE IS EMPTY", L"ERROR", MB_ICONERROR | MB_OK);
		return msg.wParam;
	}

	return EXIT_SUCCESS;
}
