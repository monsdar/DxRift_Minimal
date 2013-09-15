// include the basic windows header file
#include <windows.h>
#include <comdef.h> //provides a HRESULT toString

//STL includes
#include <fstream> //needed for the logfile

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
//Log file where we'll write debug-logs to
std::fstream logFile;

// DirectX
LPDIRECT3D9 d3d;                        // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3dDevice;            // the pointer to the device class
LPDIRECT3DVERTEXBUFFER9 vBuffer = NULL; // the pointer to the vertex buffer
LPDIRECT3DINDEXBUFFER9 iBuffer = NULL;  // the pointer to the index buffer
LPDIRECT3DVERTEXDECLARATION9 vertexDecl = NULL; //VertexDeclaration, needed for VertexShader
LPDIRECT3DVERTEXSHADER9 vertexShader = NULL; //The VertexShader
LPDIRECT3DPIXELSHADER9 pixelShader = NULL; //The PixelShader
LPD3DXCONSTANTTABLE constantTable = NULL; //Used to communicate with the Shaders
LPD3DXBUFFER code = NULL; //Buffer which holds the Shader-Code

//Oculus Rift
OVR::Ptr<OVR::DeviceManager>	pManager;
OVR::Ptr<OVR::HMDDevice>		pHMD;
OVR::Ptr<OVR::SensorDevice>		pSensor;
OVR::SensorFusion				FusionResult;


float triX = 0.0f; //East
float triY = 0.0f; //Up
float triZ = 0.0f; //South
float rotation = 0.0f;
float fov = 45.0;

// function prototypes
void initD3D(HWND hWnd); // sets up and initializes Direct3D
void initGraphics(void); // inits the graphics which we want to show during rendering
void initShaders(void);  // inits the shaders
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
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)
struct CUSTOMVERTEX
{
    FLOAT x, y, z;  // from the D3DFVF_XYZ flag
    DWORD COLOR;    // from the D3DFVF_DIFFUSE flag
};

//Initializes the rift
void initRift(void)
{
	OVR::System::Init();
	pManager = *OVR::DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();

	if (pHMD)
    {
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
        pManager.Clear();
	}

	OVR::System::Destroy();
}

//Returns the current Orientation of the Rift
OVR::Matrix4f getRiftOri(void)
{
    OVR::Quatf quaternion = FusionResult.GetPredictedOrientation();
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

    d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);    //disable culling, this will display both sides of a triangle
    d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);             // turn on the z-buffer
    d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);           // turn off the 3D lighting
    
    initGraphics();    // call the function to initialize the triangle
    initShaders(); // this inits the Pixel- and VertexShader
}

void initShaders(void)
{
    //set up vertex shader declaration
    //this is needed for the GPU to pass correct data to the VertexShader

    //TODO: This is not the right decl for our type of vertices
    D3DVERTEXELEMENT9 decl[] = {{0,
                                 0,
                                 D3DDECLTYPE_FLOAT3,
                                 D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_POSITION,
                                 0},
                                {0,
                                 12,
                                 D3DDECLTYPE_FLOAT2,
                                 D3DDECLMETHOD_DEFAULT,
                                 D3DDECLUSAGE_TEXCOORD,
                                 0},
                                D3DDECL_END()};
    d3dDevice->CreateVertexDeclaration(decl, &vertexDecl);

    //Load the actual VertexShader
    HRESULT result = D3DXCompileShaderFromFile("vertex.vsh",    //filepath
                                               NULL,            //macro's
                                               NULL,            //includes
                                               "vs_main",       //main function
                                               "vs_1_1",        //shader profile
                                               0,               //flags
                                               &code,           //compiled operations
                                               NULL,            //errors
                                               &constantTable); //constants
    
    if(result == S_OK)
    {
        logFile << "VertexShader loaded successfully" << std::endl;
        d3dDevice->CreateVertexShader(  (DWORD*)code->GetBufferPointer(), &vertexShader);
        code->Release();
    }
    else
    {
        _com_error err(result);
        logFile << "Could not load the VertexShader: " << err.ErrorMessage() << std::endl;
    }
    
    //Load the actual PixelShader
    result = D3DXCompileShaderFromFile("pixel.psh",   //filepath
                                       NULL,          //macro's            
                                       NULL,          //includes           
                                       "ps_main",     //main function      
                                       "ps_1_1",      //shader profile     
                                       0,             //flags              
                                       &code,         //compiled operations
                                       NULL,          //errors
                                       NULL);         //constants
    if(result == S_OK)
    {
        logFile << "PixelShader loaded successfully" << std::endl;
        d3dDevice->CreatePixelShader((DWORD*)code->GetBufferPointer(), &pixelShader);
        code->Release();
    }
    else
    {
        _com_error err(result);
        logFile << "Could not load the PixelShader: " << err.ErrorMessage() << std::endl;
    }
}
void initGraphics(void)
{
    // create the vertices using the CUSTOMVERTEX struct
    CUSTOMVERTEX vertices[] =
    {
        { -3.0f, 3.0f, -3.0f, D3DCOLOR_XRGB(0, 0, 255), },    // vertex 0
        { 3.0f, 3.0f, -3.0f, D3DCOLOR_XRGB(0, 255, 0), },     // vertex 1
        { -3.0f, -3.0f, -3.0f, D3DCOLOR_XRGB(255, 0, 0), },   // 2
        { 3.0f, -3.0f, -3.0f, D3DCOLOR_XRGB(0, 255, 255), },  // 3
        { -3.0f, 3.0f, 3.0f, D3DCOLOR_XRGB(0, 0, 255), },     // ...
        { 3.0f, 3.0f, 3.0f, D3DCOLOR_XRGB(255, 0, 0), },
        { -3.0f, -3.0f, 3.0f, D3DCOLOR_XRGB(0, 255, 0), },
        { 3.0f, -3.0f, 3.0f, D3DCOLOR_XRGB(0, 255, 255), },
    };

    // create a vertex buffer interface called v_buffer
    d3dDevice->CreateVertexBuffer(8*sizeof(CUSTOMVERTEX),
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
        4, 0, 6,    // side 2
        6, 0, 2,
        7, 5, 6,    // side 3
        6, 5, 4,
        3, 1, 7,    // side 4
        7, 1, 5,
        4, 5, 0,    // side 5
        0, 5, 1,
        3, 7, 2,    // side 6
        2, 7, 6,
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

// this is the function used to render a single frame
void renderFrame(void)
{
    // clear the window and Z-Buffer
    d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 200), 1.0f, 0);
    d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 200), 1.0f, 0);
    d3dDevice->BeginScene();    // begins the 3D scene

    // do 3D rendering on the back buffer here
    {
        //increase the values for Rotation and Translation of our object
        rotation += 0.02f;
        //triX += 0.1f;
        //triY += 0.1f;
        //triZ += 0.1f;

        
        // SET UP THE PIPELINE
        // set the view transform
        D3DXMATRIX camPos;
        D3DXMatrixTranslation(&camPos, 0.0f, 5.0f, 15.0f);

        OVR::Matrix4f m = getRiftOri(); //get the orientation from the rift
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

        //communicate with shader
        D3DXMATRIXA16 matWorld, matView, matProj;
        d3dDevice->GetTransform(D3DTS_WORLD, &matWorld);
        d3dDevice->GetTransform(D3DTS_VIEW, &matView);
        d3dDevice->GetTransform(D3DTS_PROJECTION, &matProj);

        D3DXMATRIXA16 matWorldViewProj = matWorld * matView * matProj;
        constantTable->SetMatrix(d3dDevice,
                                 "WorldViewProj",
                                 &matWorldViewProj);

        //add the shaders
        d3dDevice->SetVertexDeclaration(vertexDecl);
        d3dDevice->SetVertexShader(vertexShader);
        d3dDevice->SetPixelShader(pixelShader);
                
        // build a matrix to rotate the box
        D3DXMATRIX matRotateY;    // a matrix to store the rotation for the box
        D3DXMatrixRotationY(&matRotateY, rotation);    //rotate the Y-Axis (Yaw of the box)
        
        // select the vertex and index buffers to use
        d3dDevice->SetStreamSource(0, vBuffer, 0, sizeof(CUSTOMVERTEX));
        d3dDevice->SetIndices(iBuffer);

        // draw the cube
        d3dDevice->SetTransform(D3DTS_WORLD, &(matRotateY));
        d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);
    }

    d3dDevice->EndScene();    // ends the 3D scene
    d3dDevice->Present(NULL, NULL, NULL, NULL);    // displays the created frame
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
    if(constantTable)
    {
        constantTable->Release();
        constantTable = NULL;
    }
    if(pixelShader)
    {
        pixelShader->Release();
        pixelShader = NULL;
    }
    if(vertexShader)
    {
        vertexShader->Release();
        vertexShader = NULL;
    }
    if(vertexDecl)
    {
        vertexDecl->Release();
        vertexDecl = NULL;
    }
    if(iBuffer)
    {
        iBuffer->Release();
        iBuffer = NULL;
    }
    if(vBuffer)
    {
        vBuffer->Release();
        vBuffer = NULL;
    }
    if(d3dDevice)
    {
        d3dDevice->Release();
        d3dDevice = NULL;
    }
    if(d3d)
    {
        d3d->Release();
        d3d = NULL;
    }
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    //Create the logfile
    logFile.open("logfile.txt", std::fstream::out);
    logFile << "Logfile initialized" << std::endl;

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

    //close the logfile
    logFile.close();

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