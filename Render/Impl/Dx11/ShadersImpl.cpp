#include "../ShadersImpl.h"

#include "../../RenderTypes.h"
#include "RenderImpl.h"

#include <d3dcompiler.h>

#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler.lib") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
#endif

#define CS_PROFILE "cs_5_0"
#define PS_PROFILE "ps_5_0"
#define VS_PROFILE "vs_5_0"
#define GS_PROFILE "gs_5_0"

std::vector<ComPtr<ID3DBlob>>				g_vertexShaderBlobs; // Hold these for input layout creation;
std::vector<ComPtr<ID3D11VertexShader>>		g_vertexShaders;
std::vector<ComPtr<ID3D11PixelShader>>		g_pixelShaders;
std::vector<ComPtr<ID3D11GeometryShader>>	g_geometryShaders;
std::vector<ComPtr<ID3D11ComputeShader>>	g_computeShaders;

static ComPtr<ID3DBlob>& AllocVertexBlob(VertexShader_t vs)
{
	if ((size_t)vs >= g_vertexShaderBlobs.size())
		g_vertexShaderBlobs.resize((size_t)vs + 1);

	return g_vertexShaderBlobs[(size_t)vs];
}

static ComPtr<ID3D11VertexShader>& AllocVs(VertexShader_t vs)
{
	if ((size_t)vs >= g_vertexShaders.size())
		g_vertexShaders.resize((size_t)vs + 1);

	return g_vertexShaders[(size_t)vs];
}

static ComPtr<ID3D11PixelShader>& AllocPs(PixelShader_t ps)
{
	if ((size_t)ps >= g_pixelShaders.size())
		g_pixelShaders.resize((size_t)ps + 1);

	return g_pixelShaders[(size_t)ps];
}

static ComPtr<ID3D11GeometryShader>& AllocGs(GeometryShader_t gs)
{
	if ((size_t)gs >= g_geometryShaders.size())
		g_geometryShaders.resize((size_t)gs + 1);

	return g_geometryShaders[(size_t)gs];
}

static ComPtr<ID3D11ComputeShader>& AllocCs(ComputeShader_t cs)
{
	if ((size_t)cs >= g_computeShaders.size())
		g_computeShaders.resize((size_t)cs + 1);

	return g_computeShaders[(size_t)cs];
}

bool CompileShader(const char* target, const char* path, const ShaderMacros& macros, ComPtr<ID3DBlob>& shaderBlob)
{
	const size_t numMacros = macros.size();
	std::vector<D3D_SHADER_MACRO> dxMacros;
	dxMacros.resize(numMacros + 1);

	D3D_SHADER_MACRO* pdxMacros = nullptr;
	if (numMacros > 0)
	{
		for (size_t defIt = 0; defIt < numMacros; defIt++)
			dxMacros[defIt] = D3D_SHADER_MACRO({ macros[defIt]._define.c_str(), macros[defIt]._value.c_str() });

		dxMacros[numMacros] = D3D_SHADER_MACRO({ NULL, NULL });

		pdxMacros = dxMacros.data();
	}

	size_t len = strlen(path);

	assert(len < MAX_PATH && "CompileShader path length exceeds MAX_PATH");

	wchar_t wPath[MAX_PATH];

	mbstowcs_s(&len, wPath, path, len);

	ComPtr<ID3DBlob> errBlob;

	HRESULT hr = D3DCompileFromFile(
		wPath,
		pdxMacros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		target,
		0,
		0,
		&shaderBlob,
		&errBlob
	);

	if (FAILED(hr))
	{
		fprintf(stderr, "Error compiling '%s' : \n\n", path);
		if (errBlob)
		{
			fprintf(stderr, (char*)errBlob->GetBufferPointer());
		}

		return false;
	}

    return true;
}

bool CompileVertexShader(VertexShader_t handle, const char* path, const ShaderMacros& macros)
{
	AllocVertexBlob(handle);

	auto& blob = g_vertexShaderBlobs[(uint32_t)handle];

	if (!CompileShader(VS_PROFILE, path, macros, blob))
		return false;

	auto& dxVs = AllocVs(handle);

	return SUCCEEDED(g_render.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &dxVs));
}

bool CompilePixelShader(PixelShader_t handle, const char* path, const ShaderMacros& macros)
{
	ComPtr<ID3DBlob> shaderBlob;
	if (!CompileShader(PS_PROFILE, path, macros, shaderBlob))
		return false;

	auto& dxPs = AllocPs(handle);

	return SUCCEEDED(g_render.device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &dxPs));
}

bool CompileGeometryShader(GeometryShader_t handle, const char* path, const ShaderMacros& macros)
{
	ComPtr<ID3DBlob> shaderBlob;
	if(!CompileShader(GS_PROFILE, path, macros, shaderBlob))
		return false;

	auto& dxGs = AllocGs(handle);

	return SUCCEEDED(g_render.device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &dxGs));
}

bool CompileComputeShader(ComputeShader_t handle, const char* path, const ShaderMacros& macros)
{
	ComPtr<ID3DBlob> shaderBlob;

	if (!CompileShader(CS_PROFILE, path, macros, shaderBlob))
		return false;

	auto& dxCs = AllocCs(handle);

	return SUCCEEDED(g_render.device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &dxCs));
}

ID3DBlob* Dx11_GetVertexShaderBlob(VertexShader_t handle)
{
	return (size_t)handle > 0 && (size_t)handle < g_vertexShaderBlobs.size() ? g_vertexShaderBlobs[(size_t)handle].Get() : nullptr;
}

ID3D11VertexShader* Dx11_GetVertexShader(VertexShader_t handle)
{
	return (size_t)handle > 0 && (size_t)handle < g_vertexShaders.size() ? g_vertexShaders[(size_t)handle].Get() : nullptr;
}

ID3D11PixelShader* Dx11_GetPixelShader(PixelShader_t handle)
{
	return (size_t)handle > 0 && (size_t)handle < g_pixelShaders.size() ? g_pixelShaders[(size_t)handle].Get() : nullptr;
}

ID3D11GeometryShader* Dx11_GetGeometryShader(GeometryShader_t handle)
{
	return (size_t)handle > 0 && (size_t)handle < g_geometryShaders.size() ? g_geometryShaders[(size_t)handle].Get() : nullptr;
}

ID3D11ComputeShader* Dx11_GetComputeShader(ComputeShader_t handle)
{
	return (size_t)handle > 0 && (size_t)handle < g_computeShaders.size() ? g_computeShaders[(size_t)handle].Get() : nullptr;
}
