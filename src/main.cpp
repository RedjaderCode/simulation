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
#include <barrier>
#pragma comment(lib, "d2d1")

#ifndef UNICODE
#define UNICODE
#error flag -DUNICODE
#endif

ID2D1Factory* Factory               = nullptr;
ID2D1HwndRenderTarget* RenderTarget = nullptr;
ID2D1Bitmap* Bitmap                 = nullptr;

void** memory = nullptr;

uint32_t* pixelBuffer  = nullptr;
uint32_t* elementColor = nullptr;

#define TIME_DURATION 500
#define TARGET_FPS    60

#define WIDTH         800//400//320
#define HEIGHT        600//300//240

// debugging idea...

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

		~cell()
		{
			temperature  = 0.0f;
			velocity     = Vec3D(0.0f, 0.0f, 0.0f);
			pressure     = 0.0;
			active       = false;
		}

		float temperature = 0.0f;
		Vec3D velocity    = Vec3D(0.0f, 0.0f, 0.0f);
		double pressure   = 0.0;
		bool active = false;

		element MaterialType;
	};
public:
	uint32_t InitMatrix(uint32_t width, uint32_t height, uint32_t depth)
	{
		w = width; h = height; d = depth;

		// front buffer

		CELL_FRONT_BUFFER = static_cast<cell*>(malloc(sizeof(cell) * (width * height * depth)));

		for(uint32_t i=0; i<width * height * depth; ++i)
		{
			new (&CELL_FRONT_BUFFER[i]) cell(element::air);
		}

		// back buffer
		
		CELL_BACK_BUFFER = static_cast<cell*>(malloc(sizeof(cell) * (width * height * depth)));

		for(uint32_t i=0; i<width * height * depth; ++i)
		{
			new (&CELL_BACK_BUFFER[i]) cell(element::air);
		}

		matAtt = std::unique_ptr<MaterialAttributes[]>(new MaterialAttributes[static_cast<uint32_t>(element::size)]);

		printf("-> cells set to: element::air\n");
		for(uint32_t i=0; i<= width * height * depth -1; ++i)
		{
			//CELL_BACK_BUFFER[i].MaterialType = element::air;
			uint32_t x = i % w; uint32_t y = (i / w) % h; uint32_t z = i / (w * h);
			cell back_buffer_cell = CHECK_MATRIX_WALLS() ? WriteDataTo(x, y, z, cell(element::Custom)) : WriteDataTo(x, y, z, cell(element::air));
		}

		uint32_t i1 = 0;
		for(uint32_t i=FlattenedIndex(w >> 1, h >> 1, d >> 1); i<= static_cast<uint32_t>(element::size) -1; ++i)
		{
			//CELL_BACK_BUFFER[i].MaterialType = element::air;
			uint32_t x = i % w; uint32_t y = (i / w) % h; uint32_t z = i / (w * h);
			WriteDataTo(x, y, z, cell(static_cast<element>(i - FlattenedIndex(w >> 1, h >> 1, d >> 1)))); ++i1;
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
		std::swap(CELL_FRONT_BUFFER, CELL_BACK_BUFFER);
		D2D1_RECT_U rect = {0, 0, WIDTH, HEIGHT};
		Bitmap->CopyFromMemory(&rect, pixelBuffer, WIDTH * sizeof(uint32_t));
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	inline void FlushMatrix(uint32_t x, uint32_t y, uint32_t z)
	{
		cell genericCellRead = CHECK_MATRIX_WALLS() ? WriteDataTo(x, y, z, cell(element::Custom)) : cell(element::air);
		MaterialAttributes MA = ReadCellAttributes(genericCellRead); // read that these cells can't be destroyed... pretty much
	}

	// THREAD CHUNK FUNCTION //

	template<class C>
	inline void UpdateSimulationState(HWND hwnd, std::barrier<C>& SyncPoint, std::atomic<bool>* pressed, std::atomic<bool>* held, std::atomic<bool>* released, std::atomic<bool>& running, uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2, uint32_t z1, uint32_t z2)
	{
		const auto FrameDuration = std::chrono::milliseconds(1000 / TARGET_FPS);

		while(running)
		{

			auto frameStart = std::chrono::high_resolution_clock::now();

			for(uint32_t z=z1; z<=z2; ++z)
			{
				if(!running) break;

				for(uint32_t y=y1; y<=y2; ++y)
				{
					for(uint32_t x=x1; x<=x2; ++x)
					{
						FlushMatrix(x,y,z);


					}
				}
			}

			if(held[VK_TAB])
			{
				printf("Thread[%d] - Updated [(%d, %d, %d),(%d, %d, %d)]\n", std::this_thread::get_id(), x1, y1, z1, x2, y2, z2);
			}

			SyncPoint.arrive_and_wait();

			auto frameEnd = std::chrono::high_resolution_clock::now();
    		auto elapsed = frameEnd - frameStart;

			// tally the amount of time - time to finish process for throttling.
			
			if (elapsed < FrameDuration)
			{
        		std::this_thread::sleep_for(FrameDuration - elapsed);
    		}

		}
	}

	element ExGetElement(cell c){ return c.MaterialType;}

	uint32_t FlattenedIndex(uint32_t x, uint32_t y, uint32_t z)
	{
		x = x>=w ? w-1 : x; y = y>=h ? h-1 : y; z = z>=d ? d-1 : z;
		return z * (w * h) + y * w + x; 
	}
/////////////////////////////////////////////////	
	void* memoryfb          = nullptr;
	cell* CELL_FRONT_BUFFER = nullptr;

private://///////////////////////////////////////
	void* memorybb          = nullptr; 
	cell* CELL_BACK_BUFFER  = nullptr; 
/////////////////////////////////////////////////
	std::unique_ptr<MaterialAttributes[]> matAtt;
public://////////////////////////////////////////

	uint32_t w = 0;
	uint32_t h = 0;
	uint32_t d = 0;

	uint16_t _pixelWidth  = 5;
	uint16_t _pixelHeight = 5;

	float cellSize = 1.0f;

	uint16_t _zLevel = 0;

	MaterialAttributes ReadCellAttributes(cell c)
	{
		if(c.MaterialType >= element::size){ printf("\nWARNING: MaterialType: element::%d is greater than element::size. Defaulting to element::air", static_cast<uint32_t>(c.MaterialType)); }
		return c.MaterialType < element::size ? matAtt[static_cast<uint32_t>(c.MaterialType)] : matAtt[static_cast<uint32_t>(element::air)];
	}

	inline void DestroyMatrix()
	{
		for(uint32_t i=0; i<w * h * d; ++i)
		{
			CELL_FRONT_BUFFER[i].~cell();
			CELL_BACK_BUFFER[ i].~cell();
		}
		free(CELL_FRONT_BUFFER); CELL_FRONT_BUFFER = nullptr;
		free(CELL_BACK_BUFFER ); CELL_BACK_BUFFER  = nullptr;
		free(memory);                     memory   = nullptr;
	}
};

struct Camera
{
	Camera() = default;
public:

	void SetFOV(float degrees) {
        FOV = degrees;
    }

    void SetPosition(Vec3D pos) {
        position = pos;
        UpdateBasis();
    }

    void Rotate(float deltaYaw, float deltaPitch) {
        yaw += deltaYaw;
        pitch += deltaPitch;

        // Clamp pitch to avoid gimbal lock
        const float pitchLimit = 89.0f;
        if (pitch > pitchLimit) pitch = pitchLimit;
        if (pitch < -pitchLimit) pitch = -pitchLimit;

        UpdateBasis();
    }

    void MoveForward(float amount)
	{
        position = position - forward * amount;
    }

	void MoveBack(float amount)
	{
		position = position + forward * amount;
	}

    void MoveRight(float amount)
	{
        position = position + right * amount;
    }

	void MoveLeft(float amount)
	{
        position = position - right * amount;
    }

    void MoveUp(float amount)
	{
        position = position + up * amount;
    }

	void MoveDown(float amount)
	{
        position = position - up * amount;
    }

    Vec3D GetPosition() const {
        return position;
    }

    Vec3D GenerateRayDirection(int i, int j, int screenWidth, int screenHeight) const {
        float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        float fovRadians = FOV * (3.14159f / 180.0f);
        float screenHalfWidth = tanf(fovRadians / 2.0f);
        float screenHalfHeight = screenHalfWidth / aspectRatio;

        float u = (i + 0.5f) / screenWidth;
        float v = (j + 0.5f) / screenHeight;

        float screenX = (2.0f * u - 1.0f) * screenHalfWidth;
        float screenY = (1.0f - 2.0f * v) * screenHalfHeight;

        Vec3D rayDir = normalize(forward + right * screenX + up * screenY);
        return rayDir;
    }
	
private:
	void UpdateBasis()
	{
		float radYaw   = (yaw + 180.0f) * (3.14159f / 180.0f);
		float radPitch = pitch * (3.14159f / 180.0f);

		forward.x = cosf(radPitch) * sinf(radYaw);
        forward.y = sinf(radPitch);
        forward.z = cosf(radPitch) * cosf(radYaw);

		forward = normalize(forward);
		worldup  = Vec3D(0.0f, 1.0f, 0.0f);

		right = normalize(Cross(worldup, forward));
		up    = Cross(forward, right);
	}
	static Vec3D normalize(Vec3D vec)
	{
		float len = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
		return {vec.x / len, vec.y / len, vec.z / len};
	}
	static Vec3D Cross(Vec3D a, Vec3D b)
	{
	    return
		{
			a.y * b.z - a.z * b.y, 
			a.z * b.x - a.x * b.z, 
			a.x * b.y - a.y * b.x
		};
	}
protected:
	Vec3D forward  = Vec3D(0.0f, 0.0f, 1.0f);
	Vec3D right    = Vec3D(1.0f, 0.0f, 0.0f);
	Vec3D up       = Vec3D(0.0f, 1.0f, 0.0f);
	Vec3D position = Vec3D(0.0f, 0.0f, 0.0f);

	Vec3D lookAt   = Vec3D(0.0f, 0.0f, 0.0f);
	Vec3D worldup  = Vec3D(0.0f, 1.0f, 0.0f);

	float FOV      = 90.0f;
	float yaw      = 0.0f;
	float pitch    = 0.0f;
};

namespace WINDOWGraphicsOverlay
{
	inline void ErrorHandle(HRESULT hr)
	{
		if(FAILED(hr))
		{
			wchar_t errorMsg[512];
       		swprintf_s(errorMsg, L"D2D1CreateFactory failed with HRESULT: 0x%08X", hr);
       		MessageBoxW(0, errorMsg, L"Error", MB_ICONERROR);
		}
	}

	template<class C>
	bool blitOverlay(HWND hwnd, std::barrier<C>& SyncPoint, Camera& cam, std::atomic<bool>& running, uint32_t StartX, uint32_t EndX)
	{
		MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		float cellSize = matrix->cellSize;

		const auto FrameDuration = std::chrono::milliseconds(1000 / TARGET_FPS);

		while(running)
		{	
			auto frameStart = std::chrono::high_resolution_clock::now();

			for (int j = 0; j < HEIGHT; ++j)
    		{
    			for (int i = StartX; i < EndX; ++i)
    			{
					Vec3D dir = cam.GenerateRayDirection(i, j, WIDTH, HEIGHT);
					Vec3D org = cam.GetPosition();

					int ix = static_cast<int>(std::floor(org.x / cellSize));
					int iy = static_cast<int>(std::floor(org.y / cellSize));
					int iz = static_cast<int>(std::floor(org.z / cellSize));

					if (ix < 0 || iy < 0 || iz < 0 ||
					    ix >= matrix->w || iy >= matrix->h || iz >= matrix->d)
					{
					    break;
					}

					int stepX = (dir.x >= 0) ? 1 : -1;
					int stepY = (dir.y >= 0) ? 1 : -1;
					int stepZ = (dir.z >= 0) ? 1 : -1;

					//printf("ixyz[%d]%d][%d]\nstepXYZ[%d][%d][%d]\n", ix, iy, iz, stepX, stepY, stepZ);

					float invDx = (dir.x != 0) ? std::fabs(cellSize / dir.x) : std::numeric_limits<float>::infinity();
    			    float invDy = (dir.y != 0) ? std::fabs(cellSize / dir.y) : std::numeric_limits<float>::infinity();
	    	    	float invDz = (dir.z != 0) ? std::fabs(cellSize / dir.z) : std::numeric_limits<float>::infinity();

					auto firstT = [&](float posComponent, float dirComponent, int step) -> float
        			{
        			    if (dirComponent == 0) return std::numeric_limits<float>::infinity();
        			    float voxelBorder = (step > 0)
        			        ? (std::floor(posComponent / cellSize) + 1.0f) * cellSize
        			        :  std::floor(posComponent / cellSize) * cellSize;
        			    return std::fabs((voxelBorder - posComponent) / dirComponent);
        			};

					float tMaxX = firstT(org.x, dir.x, stepX);
			        float tMaxY = firstT(org.y, dir.y, stepY);
        			float tMaxZ = firstT(org.z, dir.z, stepZ);

					const float maxDist = 100.0f;   // world units
        			float traveled = 0.0f;
        			bool  hit      = false;
        			MATRIX::element mat = MATRIX::element::air;

					while (traveled < maxDist)
        			{
        			    // --- hit test ----------------------------------------------------

        			    int flat = matrix->FlattenedIndex(ix, iy, iz);
        			    if (matrix->CELL_FRONT_BUFFER[flat].MaterialType != MATRIX::element::air 
							&& matrix->CELL_FRONT_BUFFER[flat].MaterialType != MATRIX::element::Custom)
						{
        			        hit = true;
        			        mat = matrix->CELL_FRONT_BUFFER[flat].MaterialType;
        			        break;
        			    }
					
        			    // --- advance to next voxel plane --------------------------------
        			    if (tMaxX < tMaxY && tMaxX < tMaxZ)
						{
        			        ix += stepX;
        			        traveled = tMaxX;
        			        tMaxX += invDx;
        			    }
        			    else if (tMaxY < tMaxZ)
						{
        			        iy += stepY;
        			        traveled = tMaxY;
        			        tMaxY += invDy;
        			    }
        			    else {
        			        iz += stepZ;
        			        traveled = tMaxZ;
        			        tMaxZ += invDz;
        			    }
        			}

					pixelBuffer[j * WIDTH + i] = hit ? elementColor[static_cast<uint32_t>(mat)] : 0xFFFFFFFF;
				}
			}

			SyncPoint.arrive_and_wait();

			auto frameEnd = std::chrono::high_resolution_clock::now();
    		auto elapsed = frameEnd - frameStart;

			// tally the amount of time - time to finish process for throttling.
			
			if (elapsed < FrameDuration)
			{
        		std::this_thread::sleep_for(FrameDuration - elapsed);
    		}
		}
		return true;
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
			if(Bitmap)      {Bitmap->Release();      }
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
				ErrorHandle(hr);

				if(RenderTarget)
				{
					D2D1_BITMAP_PROPERTIES bmpProps = {
					    .pixelFormat = RenderTarget->GetPixelFormat(),
					    .dpiX = dpiX,
				    	.dpiY = dpiY
					};

					RenderTarget->CreateBitmap(
					    D2D1::SizeU(WIDTH, HEIGHT),
					    nullptr,
					    0, // pitch (0 = default pitch)
					    &bmpProps,
				    	&Bitmap
					);
				
					pixelBuffer  = (uint32_t*)malloc(sizeof(uint32_t) * WIDTH * HEIGHT);
					elementColor = (uint32_t*)malloc(sizeof(uint32_t) * MATRIX::element::size );
					for(int i=0; i<WIDTH * HEIGHT; ++i){ pixelBuffer[i]  = 0xFFFFFFFF; }
					for(int i=0; i<MATRIX::element::size; ++i) { elementColor[i] = 0xFFFFFFFF; }

					elementColor[static_cast<int>(MATRIX::element::air)]   = 0xFFC5F9FF;
					elementColor[static_cast<int>(MATRIX::element::water)] = 0xFF11B6FF;
					elementColor[static_cast<int>(MATRIX::element::wood)]  = 0xFF915119;
					elementColor[static_cast<int>(MATRIX::element::fire)]  = 0xFFC70000;
					elementColor[static_cast<int>(MATRIX::element::metal)] = 0xFF636363;
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
			}
			break;
			case WM_KEYDOWN:
			{
				MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				CHECK_MATRIX_POPULATION();
				
				matrix->_zLevel += wp == VK_UP   ? 1 : 0;
				matrix->_zLevel -= wp == VK_DOWN ? 1 : 0;
			}
			break;
			case WM_PAINT:
			{
				//MATRIX* matrix = reinterpret_cast<MATRIX*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				//CHECK_MATRIX_POPULATION();

				ValidateRect(hwnd, nullptr);

				if(RenderTarget && Bitmap)
				{
					RenderTarget->BeginDraw();
					RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
					RenderTarget->DrawBitmap(Bitmap, D2D1::RectF(0, 0, static_cast<float>(WIDTH), static_cast<float>(HEIGHT)));
					RenderTarget->EndDraw();
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
		Camera cam;

		//thread allocation, using smart pointers for now...
		std::unique_ptr<std::thread[]> ChunkThreads = std::unique_ptr<std::thread[]>(new std::thread[std::thread::hardware_concurrency()]);
		
		uint32_t splitPerAxis = std::round(std::cbrt(std::thread::hardware_concurrency()));
		uint32_t numofMatrixThreads = splitPerAxis * splitPerAxis * splitPerAxis;
		uint32_t threadsRemaining   = std::thread::hardware_concurrency() - numofMatrixThreads;

		auto rondevousPointFunction = [=, &running](){matrix->UpdateWorldView(_handle, running);};
		std::barrier SyncPoint(numofMatrixThreads + threadsRemaining, rondevousPointFunction);

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

					printf("MatrixThread[%d] -> indexed\n", threadIndex);

		            ChunkThreads[threadIndex++] = std::thread
					(
						[=, &SyncPoint, &keyPressed, &keyHeld, &keyReleased, &running, x1,  x2,  y1,  y2,  z1, z2](){ matrix->UpdateSimulationState(_handle, SyncPoint, keyHeld, keyHeld, keyReleased, running, x1, x2, y1, y2, z1, z2); }
					);
		        }
		    }
		}

		threadsRemaining = std::thread::hardware_concurrency() - threadIndex;

		for(int i=0; i<threadsRemaining; ++i)
		{
			int32_t StartX = i * (WIDTH / threadsRemaining);
			int32_t EndX   = i == threadsRemaining -1 ? WIDTH : StartX + (WIDTH / threadsRemaining);

			ChunkThreads[threadIndex++] = std::thread
			(
				[=, &running, &cam, &SyncPoint](){ WINDOWGraphicsOverlay::blitOverlay(_handle, SyncPoint, cam, running, StartX, EndX); }
			);
		}

		for(uint16_t i=0; i<=std::thread::hardware_concurrency() -1; ++i){
			printf("UpdateThread[%d] -> ", ChunkThreads[i].get_id());
			uint32_t n = ChunkThreads[i].get_id() != MainThreadID ? printf("NewThread\n") : printf("MainThread");
		}

		//MR.ConstructConsole(228, 180, 12, 12);
		//MR.Start(running);

		printf("Starting...\n\n");

		cam.SetFOV(90.0f);
		cam.SetPosition({10.0f, 2.5f, 10.0f});
		cam.Rotate(0.0f, -10.0f);

		const std::chrono::duration<float> FrameDuration(1.0f / TARGET_FPS);

		while(running)
		{
			auto frameStart = std::chrono::high_resolution_clock::now();

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

			auto frameEnd = std::chrono::high_resolution_clock::now();
    		float deltaTime = std::chrono::duration<float>(frameEnd - frameStart).count();

			if(keyHeld[0x57]) { cam.MoveBack(10.0f * deltaTime);    } // s
			if(keyHeld[0x53]) { cam.MoveForward(10.0f * deltaTime); } // w
			if(keyHeld[0x41]) { cam.MoveLeft(10.0f * deltaTime);    } // a
			if(keyHeld[0x44]) { cam.MoveRight(10.0f * deltaTime);   } // d
			if(keyHeld[0x20]) { cam.MoveUp(10.0f * deltaTime);      } // space
			if(keyHeld[0x10]) { cam.MoveDown(10.0f * deltaTime);    } // shift
			
			matrix->WriteDataTo(10, 2, 5, MATRIX::cell(MATRIX::element::fire));
			matrix->WriteDataTo(9,  2, 5, MATRIX::cell(MATRIX::element::metal));
			matrix->WriteDataTo(8,  2, 5, MATRIX::cell(MATRIX::element::wood));
			matrix->WriteDataTo(7,  2, 5, MATRIX::cell(MATRIX::element::water));

			matrix->WriteDataTo(10, 2, 6, MATRIX::cell(MATRIX::element::fire));
			matrix->WriteDataTo(9,  2, 6, MATRIX::cell(MATRIX::element::metal));
			matrix->WriteDataTo(8,  2, 6, MATRIX::cell(MATRIX::element::wood));
			matrix->WriteDataTo(7,  2, 6, MATRIX::cell(MATRIX::element::water));

			matrix->WriteDataTo(10, 2, 7, MATRIX::cell(MATRIX::element::fire));
			matrix->WriteDataTo(9,  2, 7, MATRIX::cell(MATRIX::element::metal));
			matrix->WriteDataTo(8,  2, 7, MATRIX::cell(MATRIX::element::wood));
			matrix->WriteDataTo(7,  2, 7, MATRIX::cell(MATRIX::element::water));

			matrix->WriteDataTo(10, 2, 8, MATRIX::cell(MATRIX::element::fire));
			matrix->WriteDataTo(9,  2, 8, MATRIX::cell(MATRIX::element::metal));
			matrix->WriteDataTo(8,  2, 8, MATRIX::cell(MATRIX::element::wood));
			matrix->WriteDataTo(7,  2, 8, MATRIX::cell(MATRIX::element::water));

			// tally the amount of time - time to finish process for throttling.
			
        	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / TARGET_FPS));
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

	matrix->DestroyMatrix();

	delete matrix;
	matrix=nullptr;
	return EXIT_SUCCESS;
}
