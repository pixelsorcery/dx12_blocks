
#include <memory>
#include "Dx12App.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

bool Dx12App::initAPI()
{
    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int antiAliasSamples = 0;
    uint flags = 0;
    return initAPI(colorFormat, depthFormat, max(antiAliasSamples, 1), 0);
}

void Dx12App::exitAPI()
{
    done = true;
    DestroyWindow(hwnd);
}

void Dx12App::setDefaultPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC * desc)
{
    ZeroMemory(desc, sizeof(*desc));
    desc->BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc->SampleMask = ~0u;
    //desc->RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    desc->RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc->RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc->RasterizerState.DepthClipEnable = TRUE;
    desc->IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    desc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc->NumRenderTargets = 1;
    desc->RTVFormats[0] = backBufferFmt;
    desc->SampleDesc.Count = 1;
}

void Dx12App::waitOnFence(ID3D12Fence *fence, UINT64 targetValue)
{
    HRESULT hr = S_OK;

    // No need to do heavier-weight synchronization if we're already past it
    if (fence->GetCompletedValue() >= targetValue)
        return;

    fence->SetEventOnCompletion(targetValue, waitEvent);
    if (FAILED(hr)) { assert(!"SetEventOnCompletion failed!");  return; }
    WaitForSingleObject(waitEvent, INFINITE);
}

void Dx12App::transitionResource(ID3D12Resource *res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = res;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter  = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    gfxCmdList->ResourceBarrier(1, &barrier);
}

ID3D12Resource* Dx12App::createBuffer(D3D12_HEAP_TYPE heap, UINT64 size, D3D12_RESOURCE_STATES states)
{
    HRESULT hr = S_OK;

    D3D12_HEAP_PROPERTIES heapProps = {};
    D3D12_RESOURCE_DESC desc = {};
    ID3D12Resource *resource;

    heapProps.Type = heap;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width     = size;
    desc.Height    = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, states, nullptr, IID_PPV_ARGS(&resource));

    if (FAILED(hr)) { assert(!"buffer creation failed!");  return 0; }

    return resource;
}

HRESULT Dx12App::uploadBuffer(ID3D12Resource *dest, UINT subresource, void const *data, UINT rowPitch, UINT slicePitch)
{
    HRESULT hr = S_OK;

    D3D12_RESOURCE_DESC desc = dest->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
    UINT64 rowSize, totalBytes;
    device->GetCopyableFootprints(&desc, subresource, 1, 0, &fp, nullptr, &rowSize, &totalBytes);

    // create upload buffer
    CComPtr<ID3D12Resource> uploadTemp;
    uploadTemp.Attach(createBuffer(D3D12_HEAP_TYPE_UPLOAD, totalBytes, D3D12_RESOURCE_STATE_GENERIC_READ));

    // map the upload resource
    BYTE *pBufData;
    hr = uploadTemp->Map(0, nullptr, (void **)&pBufData);
    if (FAILED(hr)) { assert(!"mapping upload heap failed!");  return hr; }

    // write data to upload resource
    for (UINT z = 0; z < desc.DepthOrArraySize; ++z)
    {
        BYTE const *pSource = (BYTE const *)data + z * slicePitch;
        for (UINT y = 0; y < desc.Height; ++y)
        {
            memcpy(pBufData, pSource, SIZE_T(desc.Width));
            pBufData += rowSize;
            pSource += rowPitch;
        }
    }

    // unmap 
    D3D12_RANGE written = { 0, (SIZE_T)totalBytes };
    uploadTemp->Unmap(0, &written);

    gfxCmdList->CopyResource(dest, uploadTemp);

    cmdSubmission[activeSubmission].deferredFrees.push_back((ID3D12DeviceChild *)uploadTemp);

    return hr;
}

ID3D12Resource* Dx12App::createTexture(D3D12_HEAP_TYPE heap, UINT width, UINT height, DXGI_FORMAT format)
{
    HRESULT hr = S_OK;
    D3D12_HEAP_PROPERTIES heapProps = {};
    D3D12_RESOURCE_DESC desc = {};

    heapProps.Type = heap;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    ID3D12Resource* pResource = nullptr;

    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pResource));

    ASSERT(hr == S_OK);

    return pResource;
}

void Dx12App::swapBuffers(bool vsync)
{
    HRESULT hr = S_OK;

    // submit active command buffer
    hr = gfxCmdList->Close();
    if (FAILED(hr)) { assert(!"gfxCmdList->Close() failed!");  return; }

    ID3D12CommandList  *cmds[] = { gfxCmdList };
    commandQueue->ExecuteCommandLists(1, cmds);

    // once gpu is done signal completion fence
    CmdSubmission *sub = &cmdSubmission[activeSubmission];
    sub->completionFenceVal = ++submitCount;
    hr = commandQueue->Signal(submitFence, sub->completionFenceVal);
    if (FAILED(hr)) { assert(!"commandQueue->Signal(submitFence, sub->completionFenceVal) failed!");  return; }

    // advance to the next submission in queue and make sure it's ready to use
    UINT next_submission = (activeSubmission + 1) % SUBMIT_QUEUE_DEPTH;

    if (cmdSubmission[activeSubmission].completionFenceVal)
    {
        waitOnFence(submitFence, cmdSubmission[activeSubmission].completionFenceVal);
        cmdSubmission[activeSubmission].completionFenceVal = 0;

        hr = sub->cmdAlloc->Reset();
        if (FAILED(hr)) { assert(!"sub->cmdAlloc->Reset() failed!");  return; }
    }

    cmdSubmission[activeSubmission].deferredFrees.clear();

    activeSubmission = next_submission;

    // switch cmd list over
    hr = gfxCmdList->Reset(cmdSubmission[next_submission].cmdAlloc, nullptr);
    if (FAILED(hr)) { assert(!"gfxCmdList->Reset(cmdSubmission[next_submission].cmdAlloc, nullptr) failed!");  return; }

    // present
    DXGI_PRESENT_PARAMETERS pp = { 0, nullptr, nullptr, nullptr };
    hr = swapChain->Present1(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_RESTART, &pp);
    if (FAILED(hr)) { assert(!"swapChain->Present1(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_RESTART, &pp) failed!");  return; }

    backbufCurrent = swapChain->GetCurrentBackBufferIndex();
}

bool Dx12App::initAPI(const DXGI_FORMAT backBufferFmt, const DXGI_FORMAT depthBufferFmt, const int samples, const uint flags)
{
    this->backBufferFmt = backBufferFmt;
    this->depthBufferFmt = depthBufferFmt;
    this->samples = samples;

    HRESULT hr = E_FAIL;

    bool fullscreen = false;
    uint width = m_windowWidth;
    uint height = m_windowHeight;

    IDXGIFactory1* dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) 
    { 
        ErrorMsg("Failed to create DXGI Factory");
        return false; 
    }

    IDXGIAdapter* dxgiAdapter;
    hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    if (hr == DXGI_ERROR_NOT_FOUND) 
    {
        ErrorMsg("No adapters found");
        return false;
    }

    IDXGIOutput* dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    if (hr == DXGI_ERROR_NOT_FOUND) 
    {
        ErrorMsg("No outputs found");
        return false;
    }

    DXGI_OUTPUT_DESC oDesc;
    dxgiOutput->GetDesc(&oDesc);

    DWORD wndFlags = 0;
    int x, y, w, h;

    if (fullscreen) {
        wndFlags |= WS_POPUP;
        x = y = 0;
        w = width;
        h = height;
    }
    else 
    {
        wndFlags |= WS_OVERLAPPEDWINDOW;

        RECT wRect;
        wRect.left = 0;
        wRect.right = width;
        wRect.top = 0;
        wRect.bottom = height;
        AdjustWindowRect(&wRect, wndFlags, FALSE);

        MONITORINFO monInfo;
        monInfo.cbSize = sizeof(monInfo);
        GetMonitorInfo(oDesc.Monitor, &monInfo);

        w = min(wRect.right - wRect.left, monInfo.rcWork.right - monInfo.rcWork.left);
        h = min(wRect.bottom - wRect.top, monInfo.rcWork.bottom - monInfo.rcWork.top);
        x = (monInfo.rcWork.left + monInfo.rcWork.right - w) / 2;
        y = (monInfo.rcWork.top + monInfo.rcWork.bottom - h) / 2;
    }

    hwnd = CreateWindow("lazyllama", getTitle(), wndFlags | WS_VISIBLE, x, y, w, h, HWND_DESKTOP, NULL, hInstance, NULL);

    RECT rect;
    GetClientRect(hwnd, &rect);

#if defined(_DEBUG)
    ID3D12Debug* debugController;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    if (SUCCEEDED(hr))
    {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
    else
    {
        WarningMsg("Failed to create debug interface.");
    }
#endif

    hr = D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(hr))
    {
        ErrorMsg("D3D12CreateDevice failed");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,  // D3D12_COMMAND_LIST_TYPE Type;
        0,                               // INT Priority;
        D3D12_COMMAND_QUEUE_FLAG_NONE,   // D3D12_COMMAND_QUEUE_FLAG Flags;
        0                                // UINT NodeMask;
    };

    hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create command queue");
        return false;
    }

    // TODO: add msaa support
    // create swapchain
    CComPtr<IDXGISwapChain> swapChain0;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = backBufferFmt;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    hr = dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain0);
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create swapchain");
        return false;
    }

    // Query for swapchain3 swapchain
    hr = swapChain0.QueryInterface(&swapChain);
    if (FAILED(hr)) 
    { 
        return 0; 
    }

    // Disable alt-enter
    dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

    dxgiOutput->Release();
    dxgiAdapter->Release();
    dxgiFactory->Release();

    // submission fence
    submitCount = 0;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&submitFence));
    if (FAILED(hr)) { return 0; }

    // create cbv/uav heap // TODO move this to app side not engine side or make a large generic one
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                           2,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                           0};
        ID3D12DescriptorHeap *heap;
        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
        if (FAILED(hr)) 
        { 
            return 0; 
        }

        uavHeap.Attach(heap);
    }

        // create cbv/uav heap // TODO move this to app side not engine side or make a large generic one
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                           2,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                                           0};
        ID3D12DescriptorHeap *heap;
        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
        if (FAILED(hr)) 
        { 
            return 0; 
        }

        viewDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        viewHeap.Attach(heap);
    }

    // create render target view descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                           SWAP_CHAIN_BUFFER_COUNT,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                                           0};
        ID3D12DescriptorHeap *heap;
        hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
        if (FAILED(hr)) 
        { 
            return 0; 
        }

        rtvHeap.Attach(heap);

        uavDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        uavNumAllocated = 0;
        uavHeapOffset = 0;
    }

    // prepare RTV handle base
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    UINT rtvDescHandleIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // set up RTV descriptor heap to point to backbuffers
    for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
    {
        // get the backbuffer
        hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuf[i]));
        if (FAILED(hr)) 
        { 
            return 0; 
        }

        // set up the descriptor
        backbufDescHandle[i] = rtvDescHandle;

        // create the RTV
        device->CreateRenderTargetView(backbuf[i], nullptr, rtvDescHandle);
        rtvDescHandle.ptr += rtvDescHandleIncSize;
    }

    backbufCurrent = swapChain->GetCurrentBackBufferIndex();
    backbufFormat = backBufferFmt;

    // create dsv resource
    D3D12_RESOURCE_DESC dsvDesc = {};
    dsvDesc.Format = depthBufferFmt;
    dsvDesc.Width = w;
    dsvDesc.Height = h;
    dsvDesc.MipLevels = 1;
    dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsvDesc.DepthOrArraySize = 1;
    dsvDesc.SampleDesc.Count = 1;
    dsvDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    D3D12_HEAP_PROPERTIES dsvHeapProps = {};
    dsvHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    dsvHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    dsvHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 
                                       DEFAULT_VIEW_HEAP_SIZE, 
                                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
    ID3D12DescriptorHeap *heap;
    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
    dsvHeap.Attach(heap);

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;
    clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ID3D12Resource *depthResource;
    hr = device->CreateCommittedResource(&dsvHeapProps, D3D12_HEAP_FLAG_NONE, &dsvDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&depthResource));
    if (FAILED(hr)) { return 0; }

    dsv.Attach(depthResource);

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle = {};
    dsvHeapHandle.ptr = dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    device->CreateDepthStencilView(depthResource, nullptr, dsvHeapHandle);

    // prepare submission queue // TODO: just make this an array?
    cmdSubmission = new CmdSubmission[SUBMIT_QUEUE_DEPTH];

    for (UINT i = 0; i < SUBMIT_QUEUE_DEPTH; i++)
    {
        CmdSubmission* sub = &cmdSubmission[i];

        // command allocator
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&sub->cmdAlloc));
        if (FAILED(hr)) { return 0; }
    }

    activeSubmission = 0;

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdSubmission[0].cmdAlloc, nullptr, IID_PPV_ARGS(&gfxCmdList));
    if (FAILED(hr)) { return 0; }

    // default scissor rect and viewport
    defaultScissor.left   = 0;
    defaultScissor.top    = 0;
    defaultScissor.right  = width;
    defaultScissor.bottom = height;

    defaultViewport.Height   = static_cast<float>(height);
    defaultViewport.Width    = static_cast<float>(width);
    defaultViewport.TopLeftX = 0;
    defaultViewport.TopLeftY = 0;
    defaultViewport.MaxDepth = 1.0f;
    defaultViewport.MinDepth = 0.0f;

#if 0
    // upload triangle for debugging
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 1.0f, -1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -1.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        D3D12_RESOURCE_DESC desc = {};
        desc.Alignment = 0;
        desc.DepthOrArraySize = 1;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Height = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Width = (UINT64)vertexBufferSize;

        D3D12_HEAP_PROPERTIES heapProps;
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexBuffer));

        if (FAILED(hr)) { assert(!"vb creation failed");; }

        transitionResource(vertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

        hr = uploadBuffer(vertexBuffer, 0, triangleVertices, 8, vertexBufferSize, 0);

        if (FAILED(hr)) { assert(!"vb upload failed"); }

        transitionResource(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.SizeInBytes = vertexBufferSize;
        vertexBufferView.StrideInBytes = 28;
    }
#endif 
    return true;
}
