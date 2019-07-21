#pragma once

#include <d3d12.h>
#include <atlbase.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <vector>
#include "BaseApp.h"

struct CmdSubmission
{
    CmdSubmission()
        : completionFenceVal(0)
    {
    }

    CComPtr<ID3D12CommandAllocator> cmdAlloc;

    // note 0 = not in flight
    UINT64 completionFenceVal; 

    // things to free once submission retires
    std::vector<CComPtr<ID3D12DeviceChild>> deferredFrees; 
};

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texCoord;
};

class Dx12App : public BaseApp
{
public:
    virtual bool initAPI();
    virtual void exitAPI();

    void setDefaultPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc);

    void waitOnFence(ID3D12Fence* fence, UINT64 target_value);

    void transitionResource(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

    ID3D12Resource* createBuffer(D3D12_HEAP_TYPE heap, UINT64 size, D3D12_RESOURCE_STATES states);
    HRESULT uploadBuffer(ID3D12Resource* dest, UINT subresource, void const* data, UINT rowPitch, UINT slicePitch);

    ID3D12Resource* createTexture(D3D12_HEAP_TYPE heap, UINT width, UINT height, DXGI_FORMAT format);

    void swapBuffers(bool vsync);

    virtual DXGI_FORMAT getBackBufferFmt() { return backBufferFmt; };
    virtual DXGI_FORMAT getDepthBufferFmt() { return depthBufferFmt; };

    static constexpr uint SWAP_CHAIN_BUFFER_COUNT = 4;
    static constexpr uint DEFAULT_VIEW_HEAP_SIZE  = 4096;
    static constexpr UINT SUBMIT_QUEUE_DEPTH = 3;

protected:
    bool initAPI(const DXGI_FORMAT backBufferFmt, const DXGI_FORMAT depthBufferFmt, const int samples, const uint flags);

    CComPtr<ID3D12Device>       device;
    CComPtr<ID3D12CommandQueue> commandQueue;
    CComPtr<IDXGISwapChain3>    swapChain;

    CComPtr<ID3D12GraphicsCommandList> gfxCmdList;

    CComPtr<ID3D12DescriptorHeap> rtvHeap;
    CComPtr<ID3D12Resource>       backbuf[SWAP_CHAIN_BUFFER_COUNT];
    CComPtr<ID3D12Resource>       dsv;
    D3D12_CPU_DESCRIPTOR_HANDLE   backbufDescHandle[SWAP_CHAIN_BUFFER_COUNT];
    DXGI_FORMAT                   backbufFormat;
    UINT                          backbufCurrent;

    D3D12_RECT defaultScissor;
    D3D12_VIEWPORT defaultViewport;

    // Default CBV/SRV/UAV heap // TODO instantiate when necessary
    CComPtr<ID3D12DescriptorHeap> viewHeap;
    size_t viewHeapOffset;
    UINT viewNumAllocated;
    UINT viewDescSize;

    CComPtr<ID3D12DescriptorHeap> uavHeap;
    size_t uavHeapOffset;
    uint uavNumAllocated;
    uint uavDescSize;

    CmdSubmission* cmdSubmission;
    UINT           activeSubmission;

    // Dsv heap
    CComPtr<ID3D12DescriptorHeap> dsvHeap;

    CComPtr<ID3D12Fence>    submitFence;
    UINT64                  submitCount;

    HANDLE waitEvent;
private:
    DXGI_FORMAT backBufferFmt;
    DXGI_FORMAT depthBufferFmt;
    int samples;
};