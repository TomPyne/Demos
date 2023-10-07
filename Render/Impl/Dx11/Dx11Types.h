#pragma once

#include "../../RenderTypes.h"
#include <d3d11.h>

#ifdef _MSC_VER
#pragma comment(lib, "d3d11.lib")
#endif

DXGI_FORMAT Dx11_Format(RenderFormat format);
UINT Dx11_BindFlags(RenderResourceFlags flags);
D3D11_USAGE Dx11_Usage(ResourceUsage usage);
void Dx11_CalculatePitch(DXGI_FORMAT format, UINT width, UINT height, UINT* rowPitch, UINT* slicePitch);