// include the basic windows header file
#include <windows.h>

//D3D includes
#include <d3d9.h>
#include <d3dx9.h>

//Oculus Rift
#include "OVR.h"

//////
// Constants
// define the screen resolution
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 800;

// global declarations
// DirectX
LPDIRECT3D9 d3d;                        // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3dDevice;            // the pointer to the device class
LPDIRECT3DVERTEXBUFFER9 vBuffer = NULL; // the pointer to the vertex buffer
LPDIRECT3DINDEXBUFFER9 iBuffer = NULL;  // the pointer to the index buffer

//Oculus Rift
OVR::Ptr<OVR::DeviceManager>	pManager;
OVR::Ptr<OVR::HMDDevice>		pHMD;
OVR::Ptr<OVR::SensorDevice>		pSensor;
OVR::SensorFusion				FusionResult;
OVR::HMDInfo					Info;
bool							InfoLoaded;


float triX = 0.0f; //East
float triY = 0.0f; //Up
float triZ = 0.0f; //South
float rotation = 0.0f;
float fov = 45.0;

// function prototypes
void initD3D(HWND hWnd); // sets up and initializes Direct3D
void initGraphics(void); // inits the graphics which we want to show during rendering
void initLight(void);    // sets up the light and the material
void renderFrame(void);  // renders a single frame
void cleanD3D(void);     // closes Direct3D and releases memory

void initRift(void);     //Inits the rift
void cleanRift(void);    //Closes anything rift-related
OVR::Matrix4f getRiftOri(void);//Returns the orientation of the rift

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam);

//This is our Vertex struct
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL)
struct CUSTOMVERTEX
{
    FLOAT x, y, z;    // from the D3DFVF_XYZ flag
    D3DVECTOR NORMAL; // from the D3DFVF_NORMAL flag
};

//Initializes the rift
void initRift(void)
{
	OVR::System::Init();
	pManager = *OVR::DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();

	if (pHMD)
    {
        InfoLoaded = pHMD->GetDeviceInfo(&Info);
		pSensor = *pHMD->GetSensor();
	}
	else
	{
	   pSensor = *pManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
	}

	if (pSensor)
	{
	   FusionResult.AttachToSensor(pSensor);
	}
}

//Clear up everything rift-related
void cleanRift(void)
{
	if (pSensor)
	{
		pSensor.Clear();
		pHMD.Clear();
	}

	OVR::System::Destroy();
}

//Returns the current Orientation of the Rift
OVR::Matrix4f getRiftOri(void)
{
	OVR::Quatf quaternion = FusionResult.GetOrientation();
    OVR::Matrix4f hmdMat(quaternion);
    return hmdMat;
}

// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);     // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;                // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));          // clear out the struct for use
    d3dpp.Windowed = TRUE;                      // program fullscreen, not windowed
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;   // discard old frames
    d3dpp.hDeviceWindow = hWnd;                 // set the window to be used by Direct3D
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;   // set the back buffer format to 32-bit
    d3dpp.BackBufferWidth = SCREEN_WIDTH;       // set the width of the buffer
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;     // set the height of the buffer
    d3dpp.EnableAutoDepthStencil = TRUE;    // automatically run the z-buffer for us
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;    // 16-bit pixel format for the z-buffer

    // create a device class using this information and information from the d3dpp stuct
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3dDevice);
    
    initGraphics();    // call the function to initialize the triangle
    initLight();       // call the function to initialize the light and material

    d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);    //disable culling, this will display both sides of a triangle
    d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);    // both sides of the triangles
    d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);             // turn on the z-buffer
    d3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(100, 50, 50));    // ambient light
}

void initGraphics(void)
{
    // create the vertices using the CUSTOMVERTEX struct
    CUSTOMVERTEX vertices[] =
    {
        { -3.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, },    // side 1
        { 3.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, },
        { -3.0f, 3.0f, 3.0f, 0.0f, 0.0f, 1.0f, },
        { 3.0f, 3.0f, 3.0f, 0.0f, 0.0f, 1.0f, },

        { -3.0f, -3.0f, -3.0f, 0.0f, 0.0f, -1.0f, },    // side 2
        { -3.0f, 3.0f, -3.0f, 0.0f, 0.0f, -1.0f, },
        { 3.0f, -3.0f, -3.0f, 0.0f, 0.0f, -1.0f, },
        { 3.0f, 3.0f, -3.0f, 0.0f, 0.0f, -1.0f, },

        { -3.0f, 3.0f, -3.0f, 0.0f, 1.0f, 0.0f, },    // side 3
        { -3.0f, 3.0f, 3.0f, 0.0f, 1.0f, 0.0f, },
        { 3.0f, 3.0f, -3.0f, 0.0f, 1.0f, 0.0f, },
        { 3.0f, 3.0f, 3.0f, 0.0f, 1.0f, 0.0f, },

        { -3.0f, -3.0f, -3.0f, 0.0f, -1.0f, 0.0f, },    // side 4
        { 3.0f, -3.0f, -3.0f, 0.0f, -1.0f, 0.0f, },
        { -3.0f, -3.0f, 3.0f, 0.0f, -1.0f, 0.0f, },
        { 3.0f, -3.0f, 3.0f, 0.0f, -1.0f, 0.0f, },

        { 3.0f, -3.0f, -3.0f, 1.0f, 0.0f, 0.0f, },    // side 5
        { 3.0f, 3.0f, -3.0f, 1.0f, 0.0f, 0.0f, },
        { 3.0f, -3.0f, 3.0f, 1.0f, 0.0f, 0.0f, },
        { 3.0f, 3.0f, 3.0f, 1.0f, 0.0f, 0.0f, },

        { -3.0f, -3.0f, -3.0f, -1.0f, 0.0f, 0.0f, },    // side 6
        { -3.0f, -3.0f, 3.0f, -1.0f, 0.0f, 0.0f, },
        { -3.0f, 3.0f, -3.0f, -1.0f, 0.0f, 0.0f, },
        { -3.0f, 3.0f, 3.0f, -1.0f, 0.0f, 0.0f, },
    };

    // create a vertex buffer interface called v_buffer
    d3dDevice->CreateVertexBuffer(24*sizeof(CUSTOMVERTEX),
                               0,
                               CUSTOMFVF,
                               D3DPOOL_MANAGED,
                               &vBuffer,
                               NULL);

    VOID* pVoid;    // a void pointer

    // lock v_buffer and load the vertices into it
    vBuffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, vertices, sizeof(vertices));
    vBuffer->Unlock();

    // create the indices using an int array
    short indices[] =
    {
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 5, 6,    // side 2
        6, 5, 7,
        8, 9, 10,    // side 3
        10, 9, 11,
        12, 13, 14,    // side 4
        14, 13, 15,
        16, 17, 18,    // side 5
        18, 17, 19,
        20, 21, 22,    // side 6
        22, 21, 23,
    };

    // create an index buffer interface called i_buffer
    d3dDevice->CreateIndexBuffer(36*sizeof(short),
                              0,
                              D3DFMT_INDEX16,
                              D3DPOOL_MANAGED,
                              &iBuffer,
                              NULL);

    // lock i_buffer and load the indices into it
    iBuffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, indices, sizeof(indices));
    iBuffer->Unlock();
}

// this is the function that sets up the lights and materials
void initLight(void)
{
    D3DLIGHT9 light;    // create the light struct
    ZeroMemory(&light, sizeof(light));    // clear out the light struct for use
    light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
    light.Diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);    // set the light's color
    light.Direction = D3DXVECTOR3(-1.0f, -0.3f, -1.0f);

    d3dDevice->SetLight(0, &light);    // send the light struct properties to light #0
    d3dDevice->LightEnable(0, TRUE);    // turn on light #0
    
    D3DMATERIAL9 material;    // create the material struct
    ZeroMemory(&material, sizeof(D3DMATERIAL9));    // clear out the struct for use
    material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set diffuse color to white
    material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set ambient color to white

    d3dDevice->SetMaterial(&material);    // set the globably-used material to &material
}

// this is the function used to render a single frame
void renderFrame(void)
{
    // clear the window and Z-Buffer
    d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    d3dDevice->BeginScene();    // begins the 3D scene

    // do 3D rendering on the back buffer here
    {
        // select which vertex format we are using
        d3dDevice->SetFVF(CUSTOMFVF);

        //increase the values for Rotation and Translation of our object
        //rotation += 0.05f;
        //triX += 0.1f;
        //triY += 0.1f;
        //triZ += 0.1f;

        // SET UP THE PIPELINE
        // set the view transform
        D3DXMATRIX camPos;
        D3DXMatrixTranslation(&camPos, 0.0f, 5.0f, 15.0f);

        //get the orientation from the rift
        OVR::Matrix4f m = getRiftOri();
        D3DXMATRIX camOri( m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3],
                            m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3],
                            m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3],
                            m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);

        //somehow the rift orientation is upside down... the easiest is to just apply a roll of 180°
        //I think this is caused by the rift having a slightly other coordinate system orientation then DirectX...
        D3DXMATRIX camOriFix;
        D3DXMatrixRotationZ(&camOriFix, D3DXToRadian(180.0f));

        d3dDevice->SetTransform(D3DTS_VIEW, &(camPos * camOri * camOriFix));    // set the view transform to matView

        // set the projection transform
        D3DXMATRIX matProjection;    // the projection transform matrix
        D3DXMatrixPerspectiveFovLH(&matProjection,
                                   D3DXToRadian(fov),    // the horizontal field of view
                                   (FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                                   1.0f,    // the near view-plane
                                   100.0f);    // the far view-plane
        d3dDevice->SetTransform(D3DTS_PROJECTION, &matProjection);     // set the projection
        
        // build MULTIPLE matrices to translate the model and one to rotate
        D3DXMATRIX matRotateY;    // a matrix to store the rotation for the box
        D3DXMatrixRotationY(&matRotateY, rotation);    // the front side
        
        // select the vertex and index buffers to use
        d3dDevice->SetStreamSource(0, vBuffer, 0, sizeof(CUSTOMVERTEX));
        d3dDevice->SetIndices(iBuffer);

        // draw the cube
        d3dDevice->SetTransform(D3DTS_WORLD, &(matRotateY));
        d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
    }

    d3dDevice->EndScene();    // ends the 3D scene
    d3dDevice->Present(NULL, NULL, NULL, NULL);    // displays the created frame
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
    iBuffer->Release();     // close and release the index buffer
    vBuffer->Release();     // close and release the vertex buffer
    d3dDevice->Release();   // close and release the 3D device
    d3d->Release();         // close and release Direct3D
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    // the handle for the window, filled by a function
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "TestWindowClass";

    // register the window class
    RegisterClassEx(&wc);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
                          "TestWindowClass",// name of the window class
                          "DxRift_Minimal", // title of the window
                          WS_EX_TOPMOST | WS_POPUP, // window style (Fullscreen)
                          0,                // x-position of the window
                          0,                // y-position of the window
                          SCREEN_WIDTH,     // width of the window
                          SCREEN_HEIGHT,    // height of the window
                          NULL,             // we have no parent window, NULL
                          NULL,             // we aren't using menus, NULL
                          hInstance,        // application handle
                          NULL);            // used with multiple windows, NULL

    // display the window on the screen
    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D + Rift
    initD3D(hWnd);
    initRift();

    ////////////
    // enter the main loop:
    // this struct holds Windows event messages
    MSG msg;

    // Enter the infinite message loop (GameLoop)
    while(TRUE)
    {
        // Check to see if any messages are waiting in the queue
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // Translate the message and dispatch it to WindowProc()
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If the message is WM_QUIT, exit the while loop
        if(msg.message == WM_QUIT)
            break;

        // Run game code here
        renderFrame();
    }
    
    // clean up DirectX, COM and Rift
    cleanD3D();
    cleanRift();

    // return this part of the WM_QUIT message to Windows
    return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch(message)
    {
        // this message is read when the window is closed
        case WM_DESTROY:
            {
                // close the application entirely
                PostQuitMessage(0);
                return 0;
            } break;
    }

    // Handle any messages the switch statement didn't
    return DefWindowProc (hWnd, message, wParam, lParam);
}