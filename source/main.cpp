#include "main.h"

static HWND gHwnd = nullptr;

ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gContext = nullptr;
IDXGISwapChain* gSwapChain = nullptr;
ID3D11RenderTargetView* gRTV = nullptr;
ID3D11VertexShader* gVS = nullptr;
ID3D11PixelShader* gPS = nullptr;
ID3D11BlendState* gBlend = nullptr;
IDXGIOutputDuplication* gDuplication = nullptr;
ID3D11SamplerState* gSampler = nullptr;

ID3D11Texture2D* gCapturedTexture = nullptr;
ID3D11ShaderResourceView* gSRV = nullptr;

HINSTANCE InitWindowHandler()
{
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DimScreenOverlay";
    RegisterClass(&wc);

    return hInst;
}

void CreateDimWindow(HINSTANCE hInst, HWND& hwnd)
{
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (hwnd == nullptr)
    {
        hwnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"DimScreenOverlay", L"", WS_POPUP,
            x, y, w, h,
            nullptr, nullptr, hInst, nullptr);
    }

    if (!hwnd) return;

    SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOMOVE | SWP_NOACTIVATE);
    UpdateWindow(hwnd);
}

void UpdateDimWindow(HWND& hwnd)
{
    if (!hwnd) return;

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOMOVE | SWP_NOACTIVATE);
    UpdateWindow(hwnd);
}

void CleanUpD3D()
{
    if (gBlend)
    {
        gBlend->Release();
        gBlend = nullptr;
    }

    if (gPS)
    {
        gPS->Release();
        gPS = nullptr;
    }

    if (gVS)
    {
        gVS->Release();
        gVS = nullptr;
    }

    if (gRTV)
    {
        gRTV->Release();
        gRTV = nullptr;
    }

    if (gSampler)
    {
        gSampler->Release();
        gSampler = nullptr;
    }

    if (gSRV)
    {
        gSRV->Release();
        gSRV = nullptr;
    }

    if (gCapturedTexture)
    {
        gCapturedTexture->Release();
        gCapturedTexture = nullptr;
    }

    if (gDuplication)
    {
        gDuplication->Release();
        gDuplication = nullptr;
    }

    if (gSwapChain)
    {
        gSwapChain->Release();
        gSwapChain = nullptr;
    }

    if (gContext)
    {
        gContext->ClearState();
        gContext->Flush();
        gContext->Release();
        gContext = nullptr;
    }

    if (gDevice)
    {
        gDevice->Release();
        gDevice = nullptr;
    }
}

bool ReInitD3D(HWND hwnd)
{
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	uint32_t refreshRate = Utils::GetMonitorRefreshRate();

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.BufferDesc.Width = screenW;
    sd.BufferDesc.Height = screenH;
    sd.BufferDesc.RefreshRate.Numerator = refreshRate;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &gSwapChain, &gDevice, nullptr, &gContext);

    if (FAILED(hr)) return false;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = gDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    adapter->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput1* output1 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    output->Release();
    if (FAILED(hr)) return false;

    hr = output1->DuplicateOutput(gDevice, &gDuplication);
    output1->Release();
    if (FAILED(hr))
    {
        printf("DuplicateOutput failed: %lx\n", hr);
        return false;
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = screenW;
    texDesc.Height = screenH;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    hr = gDevice->CreateTexture2D(&texDesc, nullptr, &gCapturedTexture);
    if (FAILED(hr)) return false;

    hr = gDevice->CreateShaderResourceView(gCapturedTexture, nullptr, &gSRV);
    if (FAILED(hr)) return false;

    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    hr = gDevice->CreateSamplerState(&samp, &gSampler);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backbuffer = nullptr;
    hr = gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
    if (FAILED(hr)) return false;

    hr = gDevice->CreateRenderTargetView(backbuffer, nullptr, &gRTV);
    backbuffer->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* errorBlob = nullptr;
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    if (!std::filesystem::exists(L"Effect_vertex.hlsl"))
    {
        printf("Shader file 'Effect_vertex.hlsl' not found\n");
        return false;
    }
    hr = D3DCompileFromFile(L"Effect_vertex.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) printf("%s\n", (char*)errorBlob->GetBufferPointer());
        return false;
    }

    if (!std::filesystem::exists(L"Effect_pixel.hlsl"))
    {
        printf("Shader file 'Effect_pixel.hlsl' not found\n");
        return false;
    }
    hr = D3DCompileFromFile(L"Effect_pixel.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) printf("%s\n", (char*)errorBlob->GetBufferPointer());
        return false;
    }

    hr = gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gVS);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    hr = gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gPS);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }

    vsBlob->Release();
    psBlob->Release();

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = gDevice->CreateBlendState(&bd, &gBlend);

    return SUCCEEDED(hr);
}

void Render()
{
    if (!gDuplication) return;

    bool hasNewFrame = false;

    IDXGIResource* resource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO info{};
    HRESULT hr = gDuplication->AcquireNextFrame(0, &info, &resource);

    if (SUCCEEDED(hr) && resource)
    {
        hasNewFrame = (info.AccumulatedFrames > 0) || (info.LastPresentTime.QuadPart != 0);

        if (hasNewFrame)
        {
            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex)) && tex)
            {
                gContext->CopyResource(gCapturedTexture, tex);
                tex->Release();
            }
        }

        resource->Release();
        gDuplication->ReleaseFrame();
    }
    else if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_ACCESS_DENIED || hr == DXGI_ERROR_INVALID_CALL)
    {
        UpdateDimWindow(gHwnd);
        CleanUpD3D();
        ReInitD3D(gHwnd);
        return;
    }

    static const float clear[4] = { 0, 0, 0, 0 };
    gContext->OMSetRenderTargets(1, &gRTV, nullptr);
    gContext->ClearRenderTargetView(gRTV, clear);

    gContext->OMSetBlendState(gBlend, nullptr, 0xFFFFFFFF);
    gContext->VSSetShader(gVS, nullptr, 0);
    gContext->PSSetShader(gPS, nullptr, 0);
    gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gContext->PSSetSamplers(0, 1, &gSampler);

    static int lastW = 0, lastH = 0;
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (w != lastW || h != lastH)
    {
        D3D11_VIEWPORT vp = { 0, 0, (float)w, (float)h, 0, 1 };
        gContext->RSSetViewports(1, &vp);
        lastW = w;
        lastH = h;
    }

    gContext->PSSetShaderResources(0, 1, &gSRV);
    gContext->Draw(3, 0);

    // gSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    gSwapChain->Present(1, 0); // vsync

    ID3D11ShaderResourceView* nullSRV = nullptr;
    gContext->PSSetShaderResources(0, 1, &nullSRV);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main()
{
    AllocConsole();
    (void)!freopen("CONOUT$", "w", stdout);

	HINSTANCE hInst = InitWindowHandler();
    CreateDimWindow(hInst, gHwnd);

    if (!ReInitD3D(gHwnd)) return 1;

    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Render();
    }

    return 0;
}