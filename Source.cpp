#include "Windows.h"
#include "Windowsx.h"

#include <stdio.h>
#include <string_view>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glew.h"
#include "gl/GL.h"

#include <d3d11.h>
#include <d3dcompiler.h>

HWND window = NULL;

/************************************************************/
//
/************************************************************/

typedef BOOL (WINAPI* WGLCHOOSEPIXELFORMAT      )(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
typedef HGLRC(WINAPI* WGLCREATECONTEXTATTRIBSARB)(HDC hDC, HGLRC hShareContext, const int* attribList);
typedef BOOL (WINAPI* WGLSWAPINTERVALEXT        )(int interval);

static WGLCHOOSEPIXELFORMAT       wglChoosePixelFormatARB    = NULL;
static WGLCREATECONTEXTATTRIBSARB wglCreateContextAttribsARB = NULL;

enum class GraphicsBackend
{
  OPEN_GL,
  DIRECT3D,
};

void DebugCallback(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​)
{
  printf(message​);
  printf("\n");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CLOSE:
  {
    exit(0);
    break;
  }
  default:
    break;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool RegisterWglFunctions()
{
  auto hInstance = GetModuleHandle(NULL);

  HWND window = CreateWindow(
    L"MyWindowClass",
    L"Temp",
    NULL,
    0, 0,
    0,
    0,
    NULL,
    NULL,
    GetModuleHandle(NULL),
    NULL
  );
  if (window == NULL) {
    printf("CreateWindow for temp window failed: %d", GetLastError());
    exit(-1);
  }

  HDC dc = GetDC(window);

  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

  int pixelformat = ChoosePixelFormat(dc, &pfd);
  if (SetPixelFormat(dc, pixelformat, nullptr) == FALSE) {
    printf("SetPixelFormat for temp device context failed");
    exit(-1);
  }

  HGLRC rc = wglCreateContext(dc);
  if (rc == NULL) {
    printf("CreateContext for temp device context failed");
    exit(-1);
  }

  if (!wglMakeCurrent(dc, rc)) {
    printf("Could not make temp context current");
    exit(-1);
  }

  wglChoosePixelFormatARB    = (WGLCHOOSEPIXELFORMAT      )wglGetProcAddress("wglChoosePixelFormatARB");
  wglCreateContextAttribsARB = (WGLCREATECONTEXTATTRIBSARB)wglGetProcAddress("wglCreateContextAttribsARB");

  if (!wglChoosePixelFormatARB   ) { printf("Could not get function ptr for wglChoosePixelFormatARB"   ); exit(-1); }
  if (!wglCreateContextAttribsARB) { printf("Could not get function ptr for wglCreateContextAttribsARB"); exit(-1); }

  DestroyWindow(window);

  return true;
}

bool ReadWholeFile(std::wstring path, std::string& o_str)
{
  HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  LARGE_INTEGER size;
  GetFileSizeEx(file, &size);
  o_str.resize(size.QuadPart);
  DWORD readCnt;
  if (!ReadFile(file, o_str.data(), (DWORD)o_str.size(), &readCnt, NULL)) {
    CloseHandle(file);
    return false;
  }

  CloseHandle(file);
  return true;
}

struct Vec2
{
  float x = 0;
  float y = 0;
};

struct Vertex
{
  Vec2 Position;
  Vec2 TexCoord;
};

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  //Create window and initialize OpenGL.
  HDC dc;
  {
    auto hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.cbSize        = sizeof(WNDCLASSEX);
    wndClass.style         = CS_OWNDC | CS_BYTEALIGNCLIENT | CS_DBLCLKS;
    wndClass.lpfnWndProc   = WndProc;
    wndClass.hInstance     = hInstance;
    wndClass.hIcon         = NULL;
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = L"MyWindowClass";

    ATOM reg = RegisterClassEx(&wndClass);
    if (reg == 0) {
      printf("RegisterClass failed: %d", GetLastError());
      exit(-1);
    }

    window = CreateWindow(
      L"MyWindowClass",
      L"Texture Demo",
      WS_CAPTION| WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      hInstance,
      NULL
    );
    if (window == NULL) {
      printf("CreateWindow failed: %d", GetLastError());
      exit(-1);
    }

    dc = GetDC(window);
    {
      RegisterWglFunctions();

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

      int  pixelFormats[1];
      UINT numFormats[1];
      int attribList[] =
      {
        WGL_DRAW_TO_WINDOW_ARB, (int)GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, (int)GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  (int)GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 8,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 0,
        WGL_STENCIL_BITS_ARB, 0,
        WGL_SAMPLE_BUFFERS_ARB, (int)GL_FALSE,
        WGL_SAMPLES_ARB, 0,
        0, // End
      };


      wglChoosePixelFormatARB(dc, attribList, NULL, 1, pixelFormats, numFormats);
      if (SetPixelFormat(dc, pixelFormats[0], nullptr) == FALSE) {
        printf("SetPixelFormat failed");
        exit(-1);
      }

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

      int attributes[] =
      {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
      };

      HGLRC ctx = wglCreateContextAttribsARB(dc, NULL, attributes);

      if (wglMakeCurrent(dc, ctx) == FALSE)
        printf("MakeCurrent failed: %d", GetLastError());

      //VSync
      WGLSWAPINTERVALEXT swapInterval = (WGLSWAPINTERVALEXT)wglGetProcAddress("wglSwapIntervalEXT");
      if (swapInterval){
        swapInterval(1);
      }

      glewExperimental = GL_TRUE;
      GLenum init = glewInit();
      GLenum err = glGetError();
      if (err != GL_NO_ERROR) {
        printf("glewInit failed: %d", err);
        exit(-1);
      }
      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(DebugCallback, nullptr);

    }

    ShowWindow(window, SW_MAXIMIZE);
  }

  //Load the image file with default stbi_load settings. No flipping.
  //Upper left corner will be the first pixel in the buffer.
  int texWidth, texHeight, numChannels;
  stbi_uc* texBuff = stbi_load("Texture.jpg", &texWidth, &texHeight, &numChannels, 4);

  //Quad that we can render our texture to
  const size_t NUM_VERTICES = 6;
  const Vec2 UPPER_LEFT  = { -1, +1 };
  const Vec2 UPPER_RIGHT = { +1, +1 };
  const Vec2 LOWER_LEFT  = { -1, -1 };
  const Vec2 LOWER_RIGHT = { +1, -1 };
  Vertex quadVertices[NUM_VERTICES] = {
    { LOWER_LEFT , { 0, 0 } },
    { LOWER_RIGHT, { 1, 0 } },
    { UPPER_RIGHT, { 1, 1 } },
    { UPPER_RIGHT, { 1, 1 } },
    { UPPER_LEFT , { 0, 1 } },
    { LOWER_LEFT , { 0, 0 } },
  };

  //Get the window size at the start of the application.
  //Note that we do not react to any window size changes later on.
  //Behavior when moving or resizing windows might not be as expected
  RECT rect;
  GetClientRect(window, &rect);
  int windowWidth   = rect.right  - rect.left;
  int windowHeight =  rect.bottom - rect.top ;

  float aspectRatio = (float)windowHeight / windowWidth;

  //Edit this value to switch graphics backend
  const GraphicsBackend GRAPHICS_API = GraphicsBackend::OPEN_GL;

  if (GRAPHICS_API == GraphicsBackend::OPEN_GL) {
    float viewMatrix[9] = {
      aspectRatio * 0.6f, 0, 0,
      0, 0.6f, 0,
      0, 0, 1,
    };

    GLuint quadVertexBuf = 0;
    glGenBuffers(1, &quadVertexBuf );
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuf );
    glBufferData(GL_ARRAY_BUFFER, NUM_VERTICES * sizeof(Vertex), quadVertices, GL_STATIC_DRAW);

    GLuint quadVertexArr = 0;
    glGenVertexArrays(1, &quadVertexArr);
    glBindVertexArray(quadVertexArr);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)sizeof(Vec2));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    GLuint shader = 0;
    {
      std::string vShaderContent;
      ReadWholeFile(L"./TexQuadV.glsl", vShaderContent);
      GLuint vShader = glCreateShader(GL_VERTEX_SHADER);

      const char* vSource = vShaderContent.c_str();
      glShaderSource(vShader, 1, &vSource, NULL);
      glCompileShader(vShader);

      std::string fShaderContent;
      ReadWholeFile(L"./TexQuadF.glsl", fShaderContent);
      GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

      const char* fSource = fShaderContent.c_str();
      glShaderSource(fShader, 1, &fSource, NULL);
      glCompileShader(fShader);

      shader = glCreateProgram();
      glAttachShader(shader, vShader);
      glAttachShader(shader, fShader);
      glLinkProgram(shader);

      glDeleteShader(fShader);
      glDeleteShader(vShader);
    }

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texBuff);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint shader_ViewMatrix = glGetUniformLocation(shader, "u_ViewMatrix");
    GLuint shader_Texture    = glGetUniformLocation(shader, "u_Texture");
    glUseProgram(shader);
    glUniformMatrix3fv(shader_ViewMatrix, 1, GL_FALSE, viewMatrix);

    //Not strictly necessary but we want to make sure that both implementations show the same side of the quad
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while (true) {
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      glViewport(0, 0, windowWidth, windowHeight);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texId);
      glBindVertexArray(quadVertexArr);
      glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES);
      SwapBuffers(dc);
    }
  }

  if (GRAPHICS_API == GraphicsBackend::DIRECT3D) {
    float viewMatrix[16] = {
      aspectRatio * 0.6f, 0, 0, 0,
      0, 0.6f, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
    };

    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width  = windowWidth; // use window width
    swapchaindesc.BufferDesc.Height = windowHeight; // use window height
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchaindesc.SampleDesc.Count  = 1;
    swapchaindesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchaindesc.BufferCount       = 2;
    swapchaindesc.OutputWindow      = window;
    swapchaindesc.Windowed          = TRUE;
    swapchaindesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain* swapchain;
    ID3D11Device* device;
    ID3D11DeviceContext* devicecontext;

    D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, 7, &swapchaindesc, &swapchain, &device, 0, &devicecontext);
    swapchain->GetDesc(&swapchaindesc);

    ID3D11Texture2D* rendertarget;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&rendertarget);
    ID3D11RenderTargetView* rendertargetview;
    device->CreateRenderTargetView(rendertarget, 0, &rendertargetview);

    ID3DBlob* shaderCompilationOutput;
    ID3D11VertexShader* vertexshader;
    ID3D11PixelShader* pixelshader;
    D3DCompileFromFile(L"TexQuad.hlsl", 0, 0, "vertex_shader", "vs_5_0", 0, 0, &shaderCompilationOutput, 0);
    device->CreateVertexShader(shaderCompilationOutput->GetBufferPointer(), shaderCompilationOutput->GetBufferSize(), 0, &vertexshader);


    D3D11_INPUT_ELEMENT_DESC inputelementdesc[] =
    {
      { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float2 position
      { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float2 texcoord
    };

    ID3D11InputLayout* inputlayout;
    device->CreateInputLayout(inputelementdesc, ARRAYSIZE(inputelementdesc), shaderCompilationOutput->GetBufferPointer(), shaderCompilationOutput->GetBufferSize(), &inputlayout);

    D3DCompileFromFile(L"TexQuad.hlsl", 0, 0, "pixel_shader", "ps_5_0", 0, 0, &shaderCompilationOutput, 0);
    device->CreatePixelShader(shaderCompilationOutput->GetBufferPointer(), shaderCompilationOutput->GetBufferSize(), 0, &pixelshader);

    //Not strictly necessary but we want to make sure that both implementations show the same side of the quad
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_BACK;
    rasterizerdesc.FrontCounterClockwise = TRUE;

    ID3D11RasterizerState* rasterizerstate;
    device->CreateRasterizerState(&rasterizerdesc, &rasterizerstate);

    D3D11_SAMPLER_DESC samplerdesc = {};
    samplerdesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerdesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerdesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerdesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    ID3D11SamplerState* samplerstate;
    device->CreateSamplerState(&samplerdesc, &samplerstate);

    struct Constants { float viewMatrix[16]; };

    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth      = sizeof(Constants);
    constantbufferdesc.Usage          = D3D11_USAGE_DYNAMIC;
    constantbufferdesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* constantbuffer;
    device->CreateBuffer(&constantbufferdesc, nullptr, &constantbuffer);

    D3D11_TEXTURE2D_DESC texturedesc = {};
    texturedesc.Width              = texWidth ;
    texturedesc.Height             = texHeight;
    texturedesc.MipLevels          = 1;
    texturedesc.ArraySize          = 1;
    texturedesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    texturedesc.SampleDesc.Count   = 1;
    texturedesc.Usage              = D3D11_USAGE_IMMUTABLE;
    texturedesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureSRD = {};
    textureSRD.pSysMem     = texBuff;
    textureSRD.SysMemPitch = texWidth * sizeof(UINT);

    ID3D11Texture2D* texture;

    device->CreateTexture2D(&texturedesc, &textureSRD, &texture);

    ID3D11ShaderResourceView* textureSRV;

    device->CreateShaderResourceView(texture, nullptr, &textureSRV);

    D3D11_BUFFER_DESC vertexbufferdesc = {};
    vertexbufferdesc.ByteWidth = sizeof(Vertex) * NUM_VERTICES;
    vertexbufferdesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { quadVertices };
    ID3D11Buffer* vertexbuffer;
    device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexbuffer);

    D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
    devicecontext->Map(constantbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
    {
      Constants* constants = (Constants*)constantbufferMSR.pData;
      memcpy(constants->viewMatrix, viewMatrix, sizeof(viewMatrix));
      int stop = 0;
      stop++;
    }
    devicecontext->Unmap(constantbuffer, 0);

    D3D11_DEPTH_STENCIL_DESC depthstencildesc = {};
    depthstencildesc.DepthEnable    = FALSE;
    depthstencildesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthstencildesc.DepthFunc      = D3D11_COMPARISON_LESS;

    ID3D11DepthStencilState* depthstencilstate;
    device->CreateDepthStencilState(&depthstencildesc, &depthstencilstate);




    while (true) {
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      D3D11_VIEWPORT viewport = { 0, 0, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0, 1 };
      devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      devicecontext->IASetInputLayout(inputlayout);
      unsigned int stride = sizeof(Vertex);
      unsigned int offset = 0;
      devicecontext->IASetVertexBuffers(0, 1, &vertexbuffer, &stride, &offset);
      devicecontext->VSSetShader(vertexshader, 0, 0);
      devicecontext->RSSetViewports(1, &viewport);
      devicecontext->PSSetShader(pixelshader, 0, 0);
      devicecontext->OMSetRenderTargets(1, &rendertargetview, 0);
      devicecontext->VSSetConstantBuffers(0, 1, &constantbuffer);

      float col[] = { 0, 0, 0, 1 };
      devicecontext->ClearRenderTargetView(rendertargetview, col);
      devicecontext->OMSetDepthStencilState(depthstencilstate, 0);
      devicecontext->RSSetState(rasterizerstate);
      devicecontext->PSSetSamplers(0, 1, &samplerstate);
      devicecontext->PSSetShaderResources(0, 1, &textureSRV);

      devicecontext->Draw(NUM_VERTICES, 0);
      swapchain->Present(1, 0);
    }
  }
}