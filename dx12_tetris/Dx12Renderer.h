#pragma once

#include <d3d12.h>
#include "Renderer.h"

class Dx12Renderer : public Renderer
{
public:
	Dx12Renderer(ID3D12Device *d3dDev);

protected:
	// weak ref, owned by Dx12App
	ID3D12Device *device;
};